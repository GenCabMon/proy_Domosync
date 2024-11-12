#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"

/* 
   GPIO 26/ADC0 (pin 31)-> AOUT or AUD on microphone board
   3.3v (pin 36) -> VCC on microphone board
   GND (pin 38)  -> GND on microphone board
   Reference Value for Micro - > 1.7001
*/

#define ADC_NUM 0
#define ADC_PIN (26 + ADC_NUM)
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12) // Median value reference for 12 bits register ADC
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1))
#define REF_VOLTAGE 1.7001       // Reference Value
#define MAX_SIGNAL_AMPLITUDE 1.6 // Expected max deviation from reference

#define THRESHOLD 0.25
#define SAMPLES_SIZE 5120
#define DETECTION_THRESHOLD 0.75 // Umbral de correlación para detección
#define ACTIVATION_SEQUENCE 30   // Número de muestras consecutivas por encima del umbral para activar captura

int compare_signature(const float *input_samples, const double *signature, int size);


int main()
{
    stdio_init_all();

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_NUM);

    // Configura el clock divider del ADC para lograr una frecuencia de muestreo de 8 kHz
    float clk_divider = 4800.0f; // Divisor calculado para 8 kHz
    adc_set_clkdiv(clk_divider);

    uint adc_raw;
    float normalized_value;

    float samples[SAMPLES_SIZE];
    int capture_active = 0;
    int activation_count = 0;
    int sample_index = 0;
    while (1)
    {
        adc_raw = adc_read(); // raw voltage from ADC
        normalized_value = ((adc_raw * ADC_CONVERT) - REF_VOLTAGE) / MAX_SIGNAL_AMPLITUDE;

        // Verificar si el valor excede el umbral de activación
        if (normalized_value > THRESHOLD)
        {
            activation_count++;
        }
        else
        {
            activation_count = 0; // Reiniciar el conteo si el valor cae por debajo del umbral
        }

        // Activar la captura si se alcanzó la secuencia de activación
        if (activation_count >= ACTIVATION_SEQUENCE)
        {
            capture_active = 1;
            sample_index = 0;     // Reiniciar índice de captura
            activation_count = 0; // Reiniciar el conteo de activación
        }

        // Capturar datos si la captura está activa
        if (capture_active)
        {
            printf("%.5f\n", normalized_value);
            samples[sample_index] = normalized_value;
            sample_index++;

            // Verificar si la captura está completa
            if (sample_index >= SAMPLES_SIZE)
            {
                capture_active = 0; // Desactivar la captura para procesar los datos

                printf("Termino\n");
            }
        }
    }
}

int compare_signature(const float *input_samples, const double *signature, int size) {
    float sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += input_samples[i] * signature[i]; // Correlación simple
    }
    return sum / size; // Retorna el porcentaje de similitud
}
