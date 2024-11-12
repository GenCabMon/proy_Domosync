/**
 * @file control_PI_fan.c
 * @brief Control PI discreto para un ventilador de 5V basado en la lectura de temperatura del LM35.
 */

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include <stdio.h>

// Constantes del controlador
#define KP 0.5     // Ganancia proporcional
#define KI 0.1     // Ganancia integral
#define SETPOINT 30.0 // Temperatura deseada en °C
#define PIN_PWM 10

// Variables globales
float integral = 0;
float last_error = 0;

/**
 * @brief Lee la temperatura desde el LM35 y la convierte a grados Celsius.
 * @return Temperatura en grados Celsius.
 */
float read_temperature() {
    uint16_t adc_value = adc_read();
    float voltage = (adc_value * 3.3) / 4095;
    return voltage / 0.05; // Ajusta según ganancia del amplificador.
}

/**
 * @brief Controlador PI discreto para ajustar la velocidad del ventilador.
 * @param error Error de temperatura actual.
 * @return Duty cycle ajustado (0-100).
 */
float PI_controller(float error) {
    integral += error;
    float output = KP * error + KI * integral;
    if (output > 100) output = 100;  // Limita a 100%
    if (output < 0) output = 0;      // Limita a 0%
    return output;
}

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(26); // Pin GPIO para LM35
    adc_select_input(0); // Canal 0

    while (true) {
        float temperature = read_temperature();
        float error = SETPOINT - temperature;
        float duty_cycle = PI_controller(error);
        
        // Ajustar PWM aquí usando duty_cycle
        pwm_set_gpio_level(PIN_PWM, duty_cycle * 65535 / 100); // 100% = 65535

        sleep_ms(100);
    }
}
