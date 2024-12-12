#ifndef BASEDEDATOS_H
#define BASEDEDATOS_H

// Librerías

#include <stdint.h>
#include "pico/stdlib.h"

/**
 * @brief Vector de contraseñas.
 * 
 * Este arreglo contiene las contraseñas de los usuarios en formato hexadecimal.
 * Los valores están organizados en un arreglo de tipo uint8_t.
 */
extern uint8_t vecPSWD[];

/**
 * @brief Vector de identificadores de usuarios.
 * 
 * Este arreglo contiene los identificadores únicos (IDs) de los usuarios registrados en el sistema.
 * Cada valor es un número entero de tipo uint8_t que representa un usuario.
 */
extern uint8_t vecIDs[];

/**
 * @brief Vector para almacenar los 10 dígitos ingresados por el usuario.
 * 
 * Este arreglo de tipo uint8_t almacena los 10 dígitos ingresados por el usuario en formato hexadecimal
 * para la verificación de la contraseña o acción de autenticación.
 */
extern uint8_t hKeys[10];


/**
 * @brief Contador de intentos fallidos de acceso.
 * 
 * Este arreglo de tipo uint8_t mantiene un contador de intentos fallidos de acceso por cada usuario.
 * Cada índice del arreglo corresponde a un usuario específico, y el valor indica la cantidad de intentos incorrectos.
 */
extern uint8_t missCNT[10];

/**
 * @brief Registro de IDs bloqueados.
 * 
 * Esta variable de tipo uint16_t almacena los usuarios bloqueados mediante una máscara de bits.
 * Cada bit representa a un usuario: 1 indica que el usuario está bloqueado y 0 indica que no lo está.
 */
extern uint16_t blockIDs;

#endif