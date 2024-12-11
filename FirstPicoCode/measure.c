#include <stdio.h>         /**< Librería estándar para operaciones de entrada y salida (I/O). */
#include <stdint.h>        /**< Librería estándar para tipos de datos enteros de tamaño fijo. */
#include <math.h>          /**< Librería matemática estándar, incluye funciones como potencias y raíces. */
#include <string.h>        /**< Librería estándar para manipulación de cadenas de caracteres. */
#include "pico/stdlib.h"   /**< Soporte estándar del SDK de Raspberry Pi Pico. */
#include "hardware/adc.h"  /**< Control del módulo ADC en la Raspberry Pi Pico. */
#include "hardware/gpio.h" /**< Configuración y control de pines GPIO. */
#include "hardware/irq.h"  /**< Manejo de interrupciones en el hardware. */
#include "hardware/sync.h" /**< Funciones de sincronización del hardware. */
#include "measurelibs.h"   /**< Librería personalizada para realizar mediciones específicas. */
#include "base_de_datos.h" /**< Librería personalizada para gestionar la base de datos de usuarios. */
#include "hardware/pwm.h"  /**< Control del módulo PWM en la Raspberry Pi Pico. */

/**
 * @brief Valor de referencia de voltaje para la conversión ADC.
 */
#define ADC_VREF 3.3

/**
 * @brief Rango de valores del ADC de 12 bits.
 */
#define ADC_RANGE (1 << 12) // Median value reference for 12 bits register ADC

/**
 * @brief Factor de conversión para transformar el valor raw del ADC a voltaje.
 */
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1))

/**
 * @brief Voltaje de referencia esperado.
 */
#define REF_VOLTAGE 1.7001       // Reference Value

/**
 * @brief Amplitud máxima de señal esperada en desviación desde la referencia.
 */
#define MAX_SIGNAL_AMPLITUDE 1.6 // Expected max deviation from reference


/**
 * @brief Límite de muestras a capturar en el buffer.
 */
#define CAPTURE_LIMIT 5120 // Cantidad de muestras a capturar

/**
 * @brief Umbral para iniciar la captura de muestras.
 */
#define THRESHOLD_VALUE 3000 // Umbral para iniciar captura.

/**
 * @brief Divisor de reloj para el ADC con el objetivo de lograr una frecuencia de muestreo de 8 kHz.
 */
#define ADC_CLKDIV 6000      // Divisor de reloj para lograr una FS de 8 kHz.

/**
 * @brief Número de muestras por cada captura de audio.
 */
#define SAMPLES 5120      // Numero de muestras por audio capturado

/**
 * @brief Tamaño de la ventana para realizar la Transformada de Fourier de Tiempo Corto (STFT).
 */
#define TAMANO_VENTANA 64 // Tamaño de la ventana para la STFT

/**
 * @brief Frecuencia de muestreo en Hz.
 */
#define FS 8000           // frecuencia de muestreo

/**
 * @brief Pin del LED de mesa de noche para encender y apagar.
 */
#define LED_PIN 13  // Pin led para prender y apagar

/**
 * @brief Pin del LED principal para encender y apagar.
 */
#define LED_PIN_2 0 // Pin led para prender y apagar

/**
 * @brief Pin de salida del LED del sensor del LDR.
 */
#define LED_OUT_PIN 15

/**
 * @brief Pin de salida del LED infrarrojo.
 */
#define LED_OUT_PIN_IR 14

/**
 * @brief Pin del fototransistor (LDR).
 */
#define LDR_PIN 16

/**
 * @brief Pin del sensor infrarrojo (IR).
 */
#define IR_PIN 17

/**
 * @brief Pin del botón de entrada.
 */
#define BUTTON 18

/**
 * @brief Pin del servomotor.
 */
#define Servo_PIN 19

/**
 * @brief Pin GPIO asociado al ADC.
 */
#define adc_GPIO 26

/**
 * @brief Valor para rotar el servomotor a la posición de 0°.
 */
#define ROTATE_0 1000

/**
 * @brief Valor para rotar el servomotor a la posición de 180°.
 */
#define ROTATE_180 2000

/**
 * @brief Componente entero del divisor de frecuencia del PWM.
 */
#define PWM_DIV_INTEGER 125

/**
 * @brief Componente fraccionaria del divisor de frecuencia del PWM.
 */
#define PWM_DIV_FRAC 0

/**
 * @brief Valor máximo del contador para el ciclo de trabajo del PWM.
 */
#define PWM_TOP_VALUE 19999


/**
 * @brief Ciclo de trabajo máximo permitido para el PWM.
 */
#define MAX_DUTY_CYCLE 0.1

/**
 * @brief Ciclo de trabajo mínimo permitido para el PWM.
 */
#define MIN_DUTY_CYCLE 0.05

/**
 * @brief Tiempo de anti-rebote en microsegundos.
 */
#define DEBOUNCE_TIME_US 1000000 // Tiempo de anti-rebote en microsegundos 1 seg

struct Flags  /**< Estructura para almacenar banderas del sistema. */
{
    int LDR_is_high; /**< Estado alto del sensor LDR. */
    int LDR_is_low;  /**< Estado bajo del sensor LDR. */
    int IR_is_high;  /**< Estado alto del sensor IR. */
    int IR_is_low;   /**< Estado bajo del sensor IR. */
    int is_servo;    /**< Indica si el servomotor está en uso. */
    volatile int adc_avail; /**< Indica si hay datos ADC disponibles. */
};

struct Flags Flags_1 = {0, 0, 0, 0, 0, 0};

float captured_samples[CAPTURE_LIMIT]; /**< Buffer para almacenar muestras convertidas desde el ADC. */

const int Tamano_array = SAMPLES / TAMANO_VENTANA; /**< Tamaño del arreglo para transformadas cortas. */

volatile int adc_raw = 0;       /**< Valor de la última muestra cruda del ADC. */
volatile int capture_start = 0; /**< Bandera para iniciar almacenamiento de muestras. */
volatile int capture_count = 0; /**< Contador de muestras capturadas tras cruzar el umbral. */
volatile int servo_angle = 0; /**< Ángulo actual del servomotor. */

volatile uint64_t last_interrupt_time = 0; /**< Marca de tiempo de la última interrupción. */

// Demas banderas para procesamiento
int IsProcess = 0; /**< Indica si el sistema está procesando. */
int IsShow = 0;   /**< Indica si el sistema está mostrando resultados relacionado al cambio de clave. */
int led_state = 0;   /**< Estado del LED principal, 0: apagado, 1: encendido, para alternar cmbios. */
int led_state_2 = 0; /**< Estado del LED secundario, 0: apagado, 1: encendido, para alternar cmbios. */

/**
 * @brief Inicializa los GPIOs del LED y el botón.
 */
void LandB_init();

/**
 * @brief Configura el ADC con los parámetros necesarios.
 * @param ADC_GPIO Pin GPIO utilizado como entrada para el ADC.
 */
void ADC_init(uint ADC_GPIO);

/**
 * @brief Configura el GPIO para el sensor LDR y su LED asociado.
 */
void set_up_LDR();

/**
 * @brief Configura el GPIO para el sensor IR y su LED asociado.
 */
void set_up_IR();

/**
 * @brief Maneja las interrupciones del ADC, almacena datos y actualiza banderas.
 */
void adc_handler();

/**
 * @brief Configura el tiempo de anti-rebote para evitar interrupciones frecuentes.
 */
void set_debouncing();

/**
 * @brief Callback de interrupciones para manejar eventos de GPIO.
 * @param gpio Pin GPIO que generó la interrupción.
 * @param events Evento detectado (flanco de subida o bajada).
 */
void gpio_callback(uint gpio, uint32_t events);

/**
 * @brief Inicializa el módulo PWM en un pin específico.
 * @param PWM_GPIO Pin GPIO configurado para el módulo PWM.
 */
void project_pwm_init(uint PWM_GPIO);

/**
 * @brief Establece el ángulo del servomotor mediante PWM.
 * @param PWM_GPIO Pin GPIO del servomotor.
 * @param degree Ángulo en grados para posicionar el servomotor (0 o 90).
 */
void set_servo_angle(uint PWM_GPIO, uint degree);

/**
 * @brief Función principal del programa que controla el flujo de ejecución.
 *
 * Inicializa los periféricos del sistema (LEDs, ADC, LDR, IR y PWM). 
 * Implementa la lógica para capturar muestras de audio, procesarlas y realizar
 * acciones basadas en eventos como detección de patrones mediante DTW o interacción
 * con sensores LDR e IR.
 *
 * @return 0 al finalizar correctamente.
 */
int main()
{
    // Inicializa  salida serial
    stdio_init_all();
    sleep_ms(10000); // Espera para inicializar la casa

    LandB_init();
    ADC_init(adc_GPIO);
    set_up_LDR();
    set_up_IR();

    // Initialize the PWM pin
    project_pwm_init(Servo_PIN);
    set_servo_angle(Servo_PIN, servo_angle); // Set the initial angle to 0°

    gpio_set_irq_enabled_with_callback(LDR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(IR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

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

            if ((dtw_distance_2 > 0) && (dtw_distance_2 < 3.3))
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
        }
        else if (Flags_1.LDR_is_low == 1)
        {
            gpio_put(LED_OUT_PIN, 0);
        }

        if (Flags_1.IR_is_high == 1)
        {
            gpio_put(LED_OUT_PIN_IR, 0);
        }
        else if (Flags_1.IR_is_low == 1)
        {
            gpio_put(LED_OUT_PIN_IR, 1);
        }

        if (Flags_1.is_servo)
        {
            printf("Rise\n");
            Flags_1.is_servo = 0;

            // Alternar entre 0° y 90°
            if (servo_angle == 0)
            {
                servo_angle = 90;
            }
            else
            {
                servo_angle = 0;
            }

            // Establecer el nuevo ángulo del servo
            set_servo_angle(Servo_PIN, servo_angle);
        }

        __wfi();
    }

    return 0;
}

void LandB_init()
{
    printf("Begin code\n");

    // Inicializamos el GPIO del LED como salida
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(LED_PIN_2);
    gpio_set_dir(LED_PIN_2, GPIO_OUT);

    // Initialize the GPIO input pin
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN); // Set the direction as input
    gpio_pull_up(BUTTON);          // Enable pull-up
}

void ADC_init(uint ADC_GPIO)
{
    // Configuración del ADC
    adc_init();
    adc_gpio_init(ADC_GPIO);   // GPIO 26 como entrada analógica
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

void set_debouncing()
{
    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Ignorar interrupciones si están dentro del tiempo de anti-rebote
    if (current_time - last_interrupt_time < DEBOUNCE_TIME_US)
    {
        return;
    }

    last_interrupt_time = current_time; // Actualizar el tiempo de la última interrupción
}

void gpio_callback(uint gpio, uint32_t events)
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
            Flags_1.LDR_is_high = 1; // Flanco de subida (LOW -> HIGH)
            Flags_1.LDR_is_low = 0;
        }
        else if (events & GPIO_IRQ_EDGE_FALL)
        {
            Flags_1.LDR_is_low = 1; // Flanco de bajada (HIGH -> LOW)
            Flags_1.LDR_is_high = 0;
        }
    }
    if (gpio == BUTTON)
    {

        set_debouncing();

        if (events & GPIO_IRQ_EDGE_FALL)
        { // Flanco de subida (LOW -> HIGH)
            // Mover 90 grados el servo con PWM
            Flags_1.is_servo = 1;
        }
    }
    gpio_acknowledge_irq(gpio, events);
}

void project_pwm_init(uint PWM_GPIO)
{
    gpio_init(PWM_GPIO);
    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_DIV_INTEGER);
    pwm_config_set_wrap(&cfg, PWM_TOP_VALUE);
    pwm_init(sliceNum, &cfg, true);
}

void set_servo_angle(uint PWM_GPIO, uint degree)
{
    const uint count_top = PWM_TOP_VALUE;
    float duty_cycle = (float)(MIN_DUTY_CYCLE + ((degree + 90) / 180) * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE));
    pwm_set_gpio_level(PWM_GPIO, (uint16_t)(duty_cycle * (count_top + 1)));

    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
    printf("*** PWM channel: %d ", pwm_get_counter(sliceNum));
}
