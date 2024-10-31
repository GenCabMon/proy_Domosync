#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdio.h>

#define NUM_SAMPLES 10   // Número de muestras para promediar
#define OFFSET_VALUE 0 // Ajusta esto al valor ADC medido en reposo

// Definimos los límites del rango para ruido
#define MIN_VALUE -129
#define MAX_VALUE 714

// Definir el pin del LED
#define LED_PIN 17

// Función para limitar los valores dentro del rango
int filter_value(int value);

void Isled(int value);

int main()
{
    // Inicializar el sistema
    stdio_init_all();

    // Inicializar el ADC
    adc_init();
    adc_gpio_init(26);   // Inicializar GP26 como entrada ADC
    adc_select_input(0); // Seleccionar el canal correspondiente

    // Configurar el pin LED como salida
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Bucle principal
    while (true)
    {
        uint32_t sum = 0;

        // Leer múltiples muestras para promediar(para realizar Filtro media movil)
        for (int i = 0; i < NUM_SAMPLES; i++)
        {
            uint16_t result = adc_read(); // Leer valor ADC
            sum += result;                // Sumar el resultado
            sleep_ms(10);                 // Esperar un poco entre lecturas
        }

        // Calcular el promedio
        uint16_t average = sum / NUM_SAMPLES;

        // Ajustar el valor con el offset
        int adjusted_value = average - OFFSET_VALUE;

        // Filtrar el valor y verificar si se enciende el LED
        int final_value = filter_value(adjusted_value);

        Isled(final_value);

        // Imprimir el resultado del senado
        printf("ADC Average Value: %d\n", final_value);

        //Evita que se sature
        sleep_ms(40);
    }
}

// Función para limitar los valores dentro del rango
int filter_value(int value)
{
    if (value >= MIN_VALUE && value <= MAX_VALUE)
    {
        return 0; // Si está dentro del rango, lo bajamos a 0
    }
    return value; // Si no, devolvemos el valor original
}

void Isled(int value)
{
    if (value > 1000)
    {
        gpio_put(LED_PIN, 1); // Encender el LED si el valor es mayor a 1000
    }
    else
    {
        gpio_put(LED_PIN, 0); // Apagar el LED si no es mayor a 1000
    }
}
