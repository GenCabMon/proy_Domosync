#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>       /**< Tipos de datos enteros estándar. */
#include <stdbool.h>      /**< Tipos de datos booleanos estándar. */
#include <stdio.h>        /**< Funciones estándar de entrada/salida. */
#include "hardware/timer.h" /**< Funciones para control de temporizadores de hardware. */
#include "hardware/pwm.h"   /**< Funciones para control de PWM de hardware. */

/** @def Servo_PIN
 *  @brief Pin GPIO para el control del servomotor.
 */
#define Servo_PIN 16

/** @def KP
 *  @brief Constante proporcional del controlador PID.
 */
#define KP 6 // Ganancia proporcional

/** @def KI
 *  @brief Constante integral del controlador PID.
 */
#define KI 0.3 // Ganancia integral

/** @def KD
 *  @brief Constante derivativa del controlador PID.
 */
#define KD 0.1 // Ganancia derivativa

/** @def DEBOUNCE_TIME_US
 *  @brief Tiempo de anti-rebote para el botón, en microsegundos.
 */
#define DEBOUNCE_TIME_US 3000000 // Tiempo de anti-rebote en microsegundos (300 ms)

/** @def PWM_TOP_VALUE
 *  @brief Valor máximo del temporizador PWM.
 */
#define PWM_TOP_VALUE 19999

/** @def MAX_DUTY_CYCLE
 *  @brief Máximo ciclo de trabajo permitido (10%).
 */
#define MAX_DUTY_CYCLE 0.1

/** @def MIN_DUTY_CYCLE
 *  @brief Mínimo ciclo de trabajo permitido (5%).
 */
#define MIN_DUTY_CYCLE 0.05

/** @def PWM_DIV_INTEGER
 *  @brief Divisor entero del reloj para el módulo PWM.
 */
#define PWM_DIV_INTEGER 125

extern int integral;                 /**< Valor integral acumulado para el controlador PID. */
extern float last_error;               /**< Último error registrado por el controlador PID. */
extern volatile uint64_t last_interrupt_time;     /**< Marca de tiempo de la última interrupción general. */
extern bool open;         /**< Bandera para indicar si el sistema está abierto. */


/**
 * @brief Inicializa el PWM como temporizador periódico (PIT).
 *
 * Configura un slice del PWM para generar interrupciones periódicas.
 *
 * @param slice Slice del PWM a configurar.
 * @param milis Período en milisegundos.
 * @param enable Habilita o deshabilita el slice.
 */
void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable);

/**
 * @brief Inicializa el PWM en el pin GPIO especificado.
 *
 * Configura el pin GPIO para usar PWM, ajusta el divisor de reloj y establece el valor tope.
 *
 * @param PWM_GPIO Pin GPIO que se utilizará para el PWM.
 */
void project_pwm_init(uint PWM_GPIO);

/**
 * @brief Configura el ángulo del servo en el pin GPIO especificado.
 *
 * Calcula el ciclo de trabajo en función del ángulo proporcionado.
 *
 * @param PWM_GPIO Pin GPIO asociado al servo.
 * @param degree Ángulo del servo en grados (0 o 90).
 */
void set_servo_angle(uint PWM_GPIO, uint degree);

/**
 * @brief Controlador PID que calcula la salida basada en el error actual del sensor de temperatura LM35.
 *
 * Calcula los términos proporcional, integral y derivativo para ajustar la salida.
 *
 * @param error Diferencia entre el valor deseado y el actual.
 * @return float Salida ajustada dentro del rango [0, 100].
 */
float PID_controller(float error);

/**
 * @brief Cierra la puerta principal si ha pasado el tiempo de rebote y está abierta.
 *
 * Comprueba el tiempo transcurrido desde la última interrupción y ajusta el servo
 * para cerrar la puerta si está abierta.
 */
void close();

#endif
