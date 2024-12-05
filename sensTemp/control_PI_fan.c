/**
 * @file control_PI_fan.c
 * @brief Control PI discreto para un ventilador de 5V basado en la lectura de temperatura del LM35.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/adc.h"

// Constantes del controlador
#define KP 5       // Ganancia proporcional
#define KI 0.1       // Ganancia integral
#define KD 0.2       // Ganancia derivativa
#define SETPOINT 25.0 // Temperatura deseada en °C
#define PIN_PWM 10
#define ADC_CLKDIV 6000
#define AMP_GAIN 5
#define ADC_VREF 3.3
#define ADC_RESOL 4096

// Variables globales
float integral = 0;
float last_error = 0;
volatile bool flag_adc_handler = false;
volatile uint16_t adc_raw;

/**
 * @brief Controlador PI discreto para ajustar la velocidad del ventilador.
 * @param error Error de temperatura actual.
 * @return Duty cycle ajustado (0-100).
 */
float PID_controller(float error) {
    integral += error;                      // Calcular término integral
    float derivative = error - last_error;  // Calcular término derivativo
    float output = KP * error + KI * integral + KD * derivative;

    // Limitar la salida a [0, 100]
    if (output > 100) output = 100;
    if (output < 0) output = 0;

    last_error = error; // Actualizar el error anterior
    return output;
}

void adc_handler(){
    adc_raw = adc_fifo_get();
    flag_adc_handler = true;
}

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(27); // Pin GPIO para LM35
    adc_select_input(1); // Canal 1
    adc_set_clkdiv((float)ADC_CLKDIV);
    adc_fifo_setup(
        true,  // Habilita FIFO
        false, // No usa DMA
        1,     // Umbral de FIFO en 1
        false, // No incluir errores en FIFO
        false  // No reduce resolución a 8 bits
    );
    // Configura la interrupción
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler);
    irq_set_priority(ADC_IRQ_FIFO, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_irq_set_enabled(true);
    adc_run(true);

    // Configurar PWM
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PIN_PWM);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_enabled(slice_num, true);

    while (true) {
        if(flag_adc_handler){
            float temperature = (((float)adc_raw) * ADC_VREF / (ADC_RESOL * AMP_GAIN)) * 100;
            float error = SETPOINT - temperature;
            float duty_cycle = PID_controller(error);

            // Imprimir temperatura y duty cycle al monitor serial
            printf("Temperatura: %.2f °C. Duty: %.4f. Error: %.5f", temperature, duty_cycle, error);
            // Ajustar PWM aquí usando duty_cycle
            pwm_set_gpio_level(PIN_PWM, duty_cycle * 65535 / 100); // 100% = 65535

            sleep_ms(200);
            flag_adc_handler = false;
        }
        __wfi();
    }
    return 0;
}
