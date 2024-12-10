/**
 * @file gpio_led.h
 * @version 1.0
 * @date 2024-10-07
 * @author Leyder Homero Marcillo Mera y Nelson Mauricio García López 
 * @title Secuencia de led
 * @brief Este archivo contiene la configuración de los led para el sistema de acceso.
 * */

#ifndef __GPIO_LED_H__
#define __GPIO_LED_H__

#include <stdint.h>
#include "hardware/gpio.h"

/**
 * @brief Inicializa un GPIO como salida para controlar un LED.
 * Esta función configura el pin GPIO especificado como salida y lo inicializa en estado bajo.
 * Se utiliza para controlar filas de un teclado matricial, donde los pines de las filas son configurados
 * como salidas y las columnas como entradas.
 * @param gpio_num El número del GPIO a inicializar.
 */

static inline void led_init(uint8_t gpio_num){
    gpio_init( gpio_num); // gpios for key rows 2,3,4,5
    gpio_set_dir(gpio_num,true); // rows as outputs and cols as inputs
    gpio_put(gpio_num,false);
}

/**
 * @brief Enciende el LED conectado al GPIO especificado.
 * Esta función establece el estado alto en el GPIO correspondiente, encendiendo el LED o activando
 * el dispositivo conectado al pin.
 * @param gpio_num El número del GPIO del LED que se desea encender.
 */

static inline void led_on(uint8_t gpio_num){
    gpio_put(gpio_num,true);
}

/**
 * @brief Apaga el LED conectado al GPIO especificado.
 * Esta función establece el estado bajo en el GPIO correspondiente, apagando el LED o desactivando
 * el dispositivo conectado al pin.
 * @param gpio_num El número del GPIO del LED que se desea apagar.
 */

static inline void led_off(uint8_t gpio_num){
    gpio_put(gpio_num,false);
}

/**
 * @brief Alterna el estado del LED conectado al GPIO especificado.
 * Esta función invierte el estado actual del GPIO, encendiendo el LED si estaba apagado o apagándolo si estaba encendido.
 * @param gpio_num El número del GPIO del LED cuyo estado se desea alternar.
 */

static inline void led_toggle(uint8_t gpio_num){
    gpio_xor_mask(0x00000001 << gpio_num);
}

#endif