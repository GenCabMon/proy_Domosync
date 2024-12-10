/**
 * @file access_sys.c
 * @version 1.0
 * @date 2024-10-07
 * @author Leyder Homero Marcillo Mera y Nelson Mauricio García López 
 * @title Sistema de acceso
 * @brief El sistema de control de acceso cuenta con una base de datos de usuarios, donde cada usuario tiene un 
 * identificador de ID y una contraseña asociada de 4 dígitos. Para otorgar acceso al usuario, 
 * este deberá ingresar un ID de 6 dígitos y una contraseña de 4 dígitos. El sistema buscará en su base de datos el ID,
 * y si lo encuentra, comparará la contraseña ingresada con la contraseña del usuario. Si el ID se encuentra en la base 
 * de datos y la contraseña coincide con la contraseña del usuario, entonces el sistema otorga el acceso.
 * La concesión del acceso se indicará encendiendo un LED verde durante 10 segundos. Si el ID del usuario no existe o 
 * la contraseña es incorrecta, este evento se indicará encendiendo un LED rojo durante 3 segundos. En cualquier caso, 
 * el sistema siempre debe recibir tanto el ID del usuario como su contraseña antes de indicar un intento de acceso 
 * fallido. Si un usuario con ID ingresa la contraseña incorrectamente más de 3 veces consecutivas, no necesariamente 
 * simultáneamente, el usuario es bloqueado permanentemente del sistema.
 * Otras características del sistema: El proceso de verificación de identidad tiene un tiempo máximo de 10 segundos para
 * ingresar tanto el DNI del usuario como su respectiva contraseña. Si el usuario no logra ingresar la información 
 * requerida dentro de este tiempo, el sistema regresará al estado inicial, señalando con un LED rojo que el proceso 
 * falló. Tanto el DNI como la contraseña deben ingresarse sin errores. No se pueden borrar dígitos. En caso de error, el 
 * usuario debe terminar de ingresar el total de 10 dígitos requeridos (6 DNI + 4 contraseña) o esperar a que pase el 
 * tiempo máximo de ingreso y el sistema regrese al estado inicial.
 * Un LED amarillo encendido de manera continua indicará que el sistema está listo para recibir el DNI del usuario. 
 * Cuando el usuario presiona el primer dígito de su DNI, el LED se apaga, indicando que se inició el proceso de 
 * verificación de identidad. Cuando el usuario ingresa los seis dígitos del DNI, el LED amarillo comienza a parpadear  
 * (encendido y apagado) a una frecuencia de 0.5Hz hasta que el usuario ingresa el cuarto dígito de la contraseña. 
 * En ese momento, el LED amarillo se apaga y solo se vuelve a encender cuando finaliza la señalización de contraseña 
 * correcta (LED verde - 10 * seg) o contraseña incorrecta (LED rojo - 3 seg).
 * El sistema debe tener una base de datos de al menos 10 usuarios diferentes con sus respectivas contraseñas.
 * */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "lcd_i2c.h"


uint8_t rpm = 230;
char rpm_string[4]; 

void mostrar_mensajes() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_string("Temperatura: Arrecho");
    lcd_set_cursor(1, 0);
    lcd_string("Ganas ? siempre");
    lcd_set_cursor(2, 0);
    lcd_string("Revoluciones: ");
    sprintf(rpm_string, "%d", rpm);
    lcd_string(rpm_string);
    lcd_set_cursor(3, 0);
    lcd_string("Chamo");
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Esperar 2 segundos antes de iniciar
    lcd_init(14, 15); // Inicializa el LCD en los pines SDA y SCL
    lcd_string("Iniciando...");
    sleep_ms(2000); 
    while (true) {
        mostrar_mensajes();
        sleep_ms(4000); // Cambiar mensaje cada 4 segundos
        //lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_string("Tengo Col 1");
        lcd_set_cursor(1, 0);
        lcd_string("Sueño Col 2");
        lcd_set_cursor(2, 0);
        lcd_string("pero con n 3");
        lcd_set_cursor(3, 0);
        lcd_string("QUe hora es ?");
        sleep_ms(4000); // Cambiar mensaje cada 4 segundos
    }

    return 0;
}
