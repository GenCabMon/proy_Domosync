#ifndef ACCESSSYSTEM_H
#define ACCESSSYSTEM_H

#include <stdint.h>       /**< Tipos de datos enteros estándar. */
#include <stdbool.h>      /**< Tipos de datos booleanos estándar. */
#include <stdio.h>        /**< Funciones estándar de entrada/salida. */
#include "lcd_i2c.h"       /**< Biblioteca para control de LCD mediante comunicación I2C. */

/**
 * @typedef myFlags_t
 * @brief Unión que encapsula banderas de control y su acceso mediante bits individuales.
 *
 * Esta unión permite controlar diversas banderas del sistema mediante un único valor de 16 bits.
 * El acceso se realiza a través de la estructura de bits interna o directamente mediante el valor entero.
 */
typedef union
{
    uint16_t W;
    struct
    {
        bool keyFlag : 1;   // bit 0
        bool keyDbnc : 1;   // bit 1  debouncer
        bool redLed : 1;    // bit 2  No se usa
        bool greenLed : 1;  // bit 3
        bool yellowLed : 1; // bit 4  No se usa
        bool timeOut : 1;   // bit 5  Se acabo el tiempo 10 Sg

        bool isLDR : 1;    // Flag for LDR sensor
        bool isIR : 1;     // Flag for IR sensor
        bool isMD : 1;     // Flag for main door
        bool isLamp : 1;   // Flag for Lamp
        bool isRoom : 1;   // Flag for Room
        bool isLights : 1; // Flag for Lights
        bool adcHandler : 1;

        uint8_t : 3;

    } B;
} myFlags_t;

extern volatile myFlags_t gFlags; /**< Estructura de banderas globales. */

extern bool timer_fired; /**< Bandera para indicar si pasaron 10 segundos sin ingresar una clave. */
extern bool changePas; /**< Bandera para cambiar la dinámica de la captura de dígitos, permitiendo capturar solo 4 para la nueva contraseña. */
extern volatile uint8_t gKeyCnt; /**< Contador de teclas presionadas. */
extern int8_t IsnowP; /**< Estado de clave actual del usuario. */
extern int8_t IsnowP_2; /**< Estado de la segunda clave ingresada del usuario. */
extern uint8_t accessState; /**< Estado actual del sistema de acceso. */
extern float temperature; /**< Temperatura medida por el sensor en grados Celsius. */
extern float duty_cycle; /**< Ciclo de trabajo actual del PWM. */
extern uint8_t keyPressed; /**< Última tecla presionada (0x0 a 0xF para teclas, 255 para ninguna tecla). */


/**
 * @brief Inserta una nueva tecla en la cola de teclas desplazando las existentes.
 *
 * @param key Tecla a insertar en la cola.
 */
void insertKey(uint8_t key);

/**
 * @brief Decodifica el valor del teclado a un valor correspondiente según una tabla predefinida.
 *
 * @param keyc Código de la tecla capturado.
 * @return uint8_t Valor decodificado de la tecla o 0xFF si no coincide.
 */
uint8_t keyDecode(uint32_t keyc);

/**
 * @brief Verifica si un ID específico coincide con uno en la lista de IDs.
 *
 * @param vecID Vector de IDs registrados.
 * @param ID ID a verificar.
 * @return int8_t Índice del ID en la lista o -1 si no se encuentra.
 */
int8_t checkID(uint8_t *vecID, uint8_t *ID);

/**
 * @brief Verifica si la contraseña es válida para un ID y maneja cambios de contraseña.
 *
 * @param idxID Índice del ID en la lista.
 * @param vecPSWD Vector de contraseñas registradas.
 * @param PSWD Contraseña ingresada.
 * @param IschangeP Bandera para indicar si se solicita cambio de contraseña.
 * @return true Si la contraseña es válida o si se permite el cambio.
 * @return false Si la contraseña es inválida.
 */
bool checkPSWD(int8_t idxID, uint8_t *vecPSWD, uint8_t *PSWD, uint8_t IschangeP);

/**
 * @brief Cambia la contraseña de un usuario específico.
 *
 * @param idxID Índice del ID del usuario.
 * @param vecPSWD Vector de contraseñas registradas.
 * @param PSWD Nueva contraseña a establecer.
 */
void ChangePSW(int8_t idxID, uint8_t *vecPSWD, uint8_t *PSWD);



#endif