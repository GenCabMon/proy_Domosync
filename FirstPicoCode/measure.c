#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "measurelibs.h"
#include "base_de_datos.h"

// Valores para la trasformacion del valor raw del adc
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12) // Median value reference for 12 bits register ADC
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1))
#define REF_VOLTAGE 1.7001       // Reference Value
#define MAX_SIGNAL_AMPLITUDE 1.6 // Expected max deviation from reference

// Buffer para almacenar las muestras capturadas
#define CAPTURE_LIMIT 5120 // Cantidad de muestras a capturar

// Configuración
#define THRESHOLD_VALUE 3000 // Umbral para iniciar captura.
#define ADC_CLKDIV 6000      // Divisor de reloj para lograr una FS de 8 kHz.

#define SAMPLES 5120      // Numero de muestras por audio capturado
#define TAMANO_VENTANA 64 // Tamaño de la ventana para la STFT
#define FS 8000           // frecuencia de muestreo

#define LED_PIN 13 // Pin led para prender y apagar
#define LED_PIN_2 0 // Pin led para prender y apagar

#define LED_OUT_PIN 15
#define LED_OUT_PIN_IR 14
#define LDR_PIN 16
#define IR_PIN 17

struct Flags
{
    int LDR_is_high;
    int LDR_is_low;
    int IR_is_high;
    int IR_is_low;
    volatile int adc_avail;
};

struct Flags Flags_1 = {0, 0, 0, 0, 0};

// Buffer para las muestras tomadas
float captured_samples[CAPTURE_LIMIT]; // Almacena las muestras convertidas

// Tamano para las transformadas
const int Tamano_array = SAMPLES / TAMANO_VENTANA;

// Variables globales para irq del adc
volatile int adc_raw = 0;       // Valor de la última muestra del ADC.
volatile int capture_start = 0; // Bandera para indicar cuándo comenzar a guardar.
volatile int capture_count = 0; // Contador de muestras capturadas después de cruzar el umbral.

// Demas banderas para procesamiento
int IsProcess = 0;
int IsShow = 0;
int led_state = 0;   // 0: apagado, 1: encendido
int led_state_2 = 0; // 0: apagado, 1: encendido

// Manejador de interrupción del ADC
void adc_handler();

// ISR para manejar flancos de subida y bajada
void gpio_callback_LDR(uint gpio, uint32_t events);

void gpio_callback_IR(uint gpio, uint32_t events);

void set_up_LDR();

void set_up_IR();

int main()
{
    // Inicializa  salida serial
    stdio_init_all();
    sleep_ms(10000); // Espera para inicializar la UART

    printf("Begin code\n");

    // Inicializamos el GPIO del LED como salida
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(LED_PIN_2);
    gpio_set_dir(LED_PIN_2, GPIO_OUT);

    // Configuración del ADC
    adc_init();
    adc_gpio_init(26);   // GPIO 26 como entrada analógica
    adc_select_input(0); // Selecciona el canal 0 del ADC
    adc_fifo_setup(
        true,  // Habilita FIFO
        false, // No usa DMA
        1,     // Umbral de FIFO en 1
        false, // No incluir errores en FIFO
        false  // No reduce resolución a 8 bits
    );

    // Configurar el divisor del reloj para una FS de 8 kHz (48 MHz / 6000 = 8 kHz)
    adc_set_clkdiv((float)ADC_CLKDIV);

    // Configura la interrupción
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler);
    irq_set_priority(ADC_IRQ_FIFO, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_irq_set_enabled(true);

    // Iniciar el ADC
    adc_run(true);

    set_up_LDR();
    set_up_IR();
    gpio_set_irq_enabled_with_callback(LDR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback_IR);
    gpio_set_irq_enabled_with_callback(IR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback_IR);

    // Bucle principal
    while (true)
    {

        // iniciar la captura, si valor supera el umbral
        if (!capture_start && adc_raw >= THRESHOLD_VALUE)
        {
            capture_start = 1; // Activa la bandera para iniciar la captura
        }

        // Si la captura ha comenzado, guarda las muestras
        if ((capture_start) && (Flags_1.adc_avail))
        {
            captured_samples[capture_count] = ((adc_raw * ADC_CONVERT) - REF_VOLTAGE) / MAX_SIGNAL_AMPLITUDE;
            capture_count++;
            Flags_1.adc_avail = 0;
        }

        // Verifica si se alcanzaron las 5120 muestras
        if ((capture_count >= CAPTURE_LIMIT) && !IsProcess)
        {
            // Imprime las muestras almacenadas
            for (int i = 0; i < CAPTURE_LIMIT; i++)
            {
                printf("%.5f\n", captured_samples[i]);
            }
            printf("Cantidad de muestras: %d\n", capture_count);
            IsProcess = 1;
        }

        if (IsProcess && !IsShow)
        {
            // Calcular la amplitud promedio y los índices de tiempo para cada ventana
            float amplitudes_promedio[Tamano_array];   // Array para almacenar las amplitudes promedio
            float indices_tiempo[Tamano_array];        // Array para almacenar los índices de tiempo
            float amplitudes_promedio_2[Tamano_array]; // Array para almacenar las amplitudes promedio
            float indices_tiempo_2[Tamano_array];      // Array para almacenar los índices de tiempo

            float amplitudes_promedio_3[Tamano_array]; // Array para almacenar las amplitudes promedio
            float indices_tiempo_3[Tamano_array];      // Array para almacenar los índices de tiempo
            float amplitudes_promedio_4[Tamano_array]; // Array para almacenar las amplitudes promedio
            float indices_tiempo_4[Tamano_array];      // Array para almacenar los índices de tiempo

            graficar_amplitud_promedio_frecuencia(Datos_tres_aplausos_1, FS, TAMANO_VENTANA, amplitudes_promedio, indices_tiempo);
            graficar_amplitud_promedio_frecuencia(captured_samples, FS, TAMANO_VENTANA, amplitudes_promedio_2, indices_tiempo_2);

            graficar_amplitud_promedio_frecuencia(Datos_dos_aplausos_1, FS, TAMANO_VENTANA, amplitudes_promedio_3, indices_tiempo_3);
            graficar_amplitud_promedio_frecuencia(captured_samples, FS, TAMANO_VENTANA, amplitudes_promedio_4, indices_tiempo_4);

            float dtw_distance = dtw(amplitudes_promedio, 80, amplitudes_promedio_2, 80);

            float dtw_distance_2 = dtw(amplitudes_promedio_3, 80, amplitudes_promedio_4, 80);

            printf("Distancia DTW tres aplausos: %.4f\n", dtw_distance);

            printf("Distancia DTW dos aplausos: %.4f\n", dtw_distance_2);
            IsShow = 1;

            if ((dtw_distance > 0) && (dtw_distance < 4))
            {
                led_state = !led_state;       // Cambiar el estado del LED
                gpio_put(LED_PIN, led_state); // Actualizar el estado del LED
            }

            if ((dtw_distance_2 > 0) && (dtw_distance_2 < 3))
            {
                led_state_2 = !led_state_2;       // Cambiar el estado del LED
                gpio_put(LED_PIN_2, led_state_2); // Actualizar el estado del LED
            }   

            // Reiniciar las banderas y el buffer
            Flags_1.adc_avail = 0;
            capture_start = 0; // Bandera para indicar cuándo comenzar a guardar.
            capture_count = 0; // Contador de muestras capturadas después de cruzar el umbral.
            IsProcess = 0;
            IsShow = 0;

            // Limpiar el buffer de muestras capturadas
            memset(captured_samples, 0, sizeof(captured_samples));
        }

        if (Flags_1.LDR_is_high == 1)
        {
            gpio_put(LED_OUT_PIN, 1);
            printf("Se prende\n");
        }
        else if (Flags_1.LDR_is_low == 1)
        {
            gpio_put(LED_OUT_PIN, 0);
            printf("Se apaga\n");
        }

        if (Flags_1.IR_is_high == 1)
        {
            gpio_put(LED_OUT_PIN_IR, 0);
        }
        else if (Flags_1.IR_is_low == 1)
        {
            gpio_put(LED_OUT_PIN_IR, 1);
        }

        __wfi();
    }

    return 0;
}

void adc_handler()
{
    // Lee la muestra y marca como disponible
    adc_raw = adc_fifo_get();
    Flags_1.adc_avail = 1;

    if (capture_count >= CAPTURE_LIMIT)
    {
        Flags_1.adc_avail = 0;
    }
}

void gpio_callback_LDR(uint gpio, uint32_t events)
{
    if (gpio == LDR_PIN)
    {
        if (events & GPIO_IRQ_EDGE_RISE)
        {
            printf("Entra alto\n");
            Flags_1.LDR_is_high = 1; // Flanco de subida (LOW -> HIGH)
            Flags_1.LDR_is_low = 0;
        }
        else if (events & GPIO_IRQ_EDGE_FALL)
        {
            printf("Entra bajo\n");
            Flags_1.LDR_is_low = 1; // Flanco de bajada (HIGH -> LOW)
            Flags_1.LDR_is_high = 0;
        }
    }
    gpio_acknowledge_irq(gpio, events);
}

void gpio_callback_IR(uint gpio, uint32_t events)
{
    if (gpio == IR_PIN)
    {
        if (events & GPIO_IRQ_EDGE_RISE)
        {
            Flags_1.IR_is_high = 1; // Flanco de subida (LOW -> HIGH)
            Flags_1.IR_is_low = 0;
        }
        else if (events & GPIO_IRQ_EDGE_FALL)
        {
            Flags_1.IR_is_low = 1; // Flanco de bajada (HIGH -> LOW)
            Flags_1.IR_is_high = 0;
        }
    }

    if (gpio == LDR_PIN)
    {
        if (events & GPIO_IRQ_EDGE_RISE)
        {
            printf("Entra alto\n");
            Flags_1.LDR_is_high = 1; // Flanco de subida (LOW -> HIGH)
            Flags_1.LDR_is_low = 0;
        }
        else if (events & GPIO_IRQ_EDGE_FALL)
        {
            printf("Entra bajo\n");
            Flags_1.LDR_is_low = 1; // Flanco de bajada (HIGH -> LOW)
            Flags_1.LDR_is_high = 0;
        }
    }
    gpio_acknowledge_irq(gpio, events);
}

void set_up_LDR()
{
    // Initialize the GPIO input pin
    gpio_init(LDR_PIN);
    gpio_set_dir(LDR_PIN, GPIO_IN); // Set the direction as input
    gpio_pull_down(LDR_PIN);        // Enable pull-up

    // Initialize the GPIO out pin
    gpio_init(LED_OUT_PIN);
    gpio_set_dir(LED_OUT_PIN, GPIO_OUT); // Set the direction as output
}

void set_up_IR()
{
    // Initialize the GPIO input pin
    gpio_init(IR_PIN);
    gpio_set_dir(IR_PIN, GPIO_IN); // Set the direction as input
    gpio_pull_down(IR_PIN);        // Enable pull-up

    // Initialize the GPIO out pin
    gpio_init(LED_OUT_PIN_IR);
    gpio_set_dir(LED_OUT_PIN_IR, GPIO_OUT); // Set the direction as output
}
