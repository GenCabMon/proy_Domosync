#ifndef CONFIGPWM_H
#define CONFIGPWM_H


// Librerías

#include "pico/stdlib.h"    /**< Soporte estándar del SDK de Raspberry Pi Pico. */
#include "hardware/pwm.h"   /**< Control del módulo PWM en la Raspberry Pi Pico. */
#include "hardware/gpio.h"  /**< Configuración y control de pines GPIO. */
#include <stdio.h>          /**< Librería estándar para operaciones de entrada y salida (I/O). */
#include <stdint.h>         /**< Librería estándar para tipos de datos enteros de tamaño fijo. */
#include "hardware/sync.h"  /**< Funciones de sincronización del hardware. */

// Macros

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

/**
 * @brief Pin del servomotor.
 */
#define Servo_PIN 19


extern volatile uint64_t last_interrupt_time; /**< Marca de tiempo de la última interrupción. */
// Funciones

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
 * @brief Configura el tiempo de anti-rebote para evitar interrupciones frecuentes.
 */
void set_debouncing();

#endif