#ifndef DIGIELEMENTS_H
#define DIGIELEMENTS_H

#include "hardware/gpio.h"  /**< Configuración y control de pines GPIO. */
#include "hardware/adc.h"   /**< Control del módulo ADC en la Raspberry Pi Pico. */
#include "hardware/irq.h"   /**< Manejo de interrupciones en el hardware. */
#include <stdio.h>          /**< Librería estándar para operaciones de entrada y salida (I/O). */
#include <stdint.h>         /**< Librería estándar para tipos de datos enteros de tamaño fijo. */
#include "pico/stdlib.h"    /**< Soporte estándar del SDK de Raspberry Pi Pico. */
#include "hardware/sync.h"  /**< Funciones de sincronización del hardware. */

// Macros de inicialización de sensores digitales

/**
 * @brief Pin del botón de entrada.
 */
#define BUTTON 18

/**
 * @brief Divisor de reloj para el ADC con el objetivo de lograr una frecuencia de muestreo de 8 kHz.
 */
#define ADC_CLKDIV 6000      // Divisor de reloj para lograr una FS de 8 kHz.

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

// Funciones de Inicialización de sensores

/**
 * @brief Inicializa los GPIOs del LED y el botón.
 */
void LandB_init();

/**
 * @brief Configura el GPIO para el sensor LDR y su LED asociado.
 */
void set_up_LDR();

/**
 * @brief Configura el GPIO para el sensor IR y su LED asociado.
 */
void set_up_IR();

#endif