#include "access_system.h"
#include "base_de_datos.h"
#include <stdint.h>       /**< Tipos de datos enteros estándar. */
#include <stdbool.h>      /**< Tipos de datos booleanos estándar. */
#include <stdio.h>        /**< Funciones estándar de entrada/salida. */

volatile myFlags_t gFlags;
bool timer_fired = false; /**< Bandera para indicar si pasaron 10 segundos sin ingresar una clave. */
bool changePas= false; /**< Bandera para cambiar la dinámica de la captura de dígitos, permitiendo capturar solo 4 para la nueva contraseña. */
volatile uint8_t gKeyCnt = 0; /**< Contador de teclas presionadas. */
int8_t IsnowP = 0; /**< Estado de clave actual del usuario. */
int8_t IsnowP_2 = 0; /**< Estado de la segunda clave ingresada del usuario. */
uint8_t accessState = 0; /**< Estado actual del sistema de acceso. */
float temperature = 0; /**< Temperatura medida por el sensor en grados Celsius. */
float duty_cycle = 0; /**< Ciclo de trabajo actual del PWM. */
uint8_t keyPressed = 255; /**< Última tecla presionada (0x0 a 0xF para teclas, 255 para ninguna tecla). */

void insertKey(uint8_t key)
{
    for (int i = 9; i > 0; i--)
    {
        hKeys[i] = hKeys[i - 1];
    }
    hKeys[0] = key;
}

uint8_t keyDecode(uint32_t keyc)
{
    uint8_t keyd = 0xFF;
    switch (keyc)
    {
    case 0x88:
        keyd = 0x01;
        break;
    case 0x48:
        keyd = 0x02;
        break;
    case 0x28:
        keyd = 0x03;
        break;
    case 0x18:
        keyd = 0x0A;
        break;
    case 0x84:
        keyd = 0x04;
        break;
    case 0x44:
        keyd = 0x05;
        break;
    case 0x24:
        keyd = 0x06;
        break;
    case 0x14:
        keyd = 0x0B;
        break;
    case 0x82:
        keyd = 0x07;
        break;
    case 0x42:
        keyd = 0x08;
        break;
    case 0x22:
        keyd = 0x09;
        break;
    case 0x12:
        keyd = 0x0C;
        break;
    case 0x81:
        keyd = 0x0E;
        break;
    case 0x41:
        keyd = 0x00;
        break;
    case 0x21:
        keyd = 0x0F;
        break;
    case 0x11:
        keyd = 0x0D;
        break;
    default:
        keyd = 0xFF;
        break;
    }
    return keyd;
}

int8_t checkID(uint8_t *vecID, uint8_t *ID)
{
    for (int i = 0; i < 10; i++)
    {
        bool flag = true;
        for (int j = 0; j < 6; j++)
        {
            if (vecID[6 * i + j] != ID[j])
            {
                flag = false;
                break;
            }
        }
        if (flag == true)
        {
            //printf("Se encontró usuario con ID %d\n", i);
            return i;
        }
    }
    return -1;
}

bool checkPSWD(int8_t idxID, uint8_t *vecPSWD, uint8_t *PSWD, uint8_t IschangeP)
{
    if (hKeys[0] == 0xF && hKeys[1] == 0xF && hKeys[2] == 0xF && hKeys[3] == 0xF)
    { // verifica si quiere cambiar contrasena el usuario (FFFF)
        // Si la comparación es verdadera, hacer algo
        timer_fired = false;
        changePas = true; // Indica que los 4 valores siguientes a capturar son la nueva contrasena
        // led_on(GREEN_LED);
        // led_on(RED_LED);
        //printf("Ingrese su ID y la actual contraseña\n");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_string("Ingrese su ID");
        lcd_set_cursor(1, 0);
        lcd_string("y su clave actual");
        gKeyCnt = 0;
        hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
        //gFlags.B.greenLed = false;
        
    }
    else if (!IsnowP)
    {
        for (int j = 0; j < 4; j++)
        {
            if (vecPSWD[4 * idxID + j] != PSWD[j])
            {
                return false;
            }
        }
    }

    if (IschangeP)
    {
        if (IsnowP)
        {
            IsnowP_2 = 1;
        }
        //printf("Reconocido");
        IsnowP = 1;
        return true;
    }

    return true;
}

void ChangePSW(int8_t idxID, uint8_t *vecPSWD, uint8_t *PSWD)
{
    int cont_F = 0;
    for (int j = 0; j < 4; j++)
    {
        if (PSWD[j] == 0xF)
        {
            //printf("entro");
            cont_F++;
        }
    }
    //printf("CONTF ES: %d\n", cont_F);
    if (cont_F < 4)
    {
        for (int j = 0; j < 4; j++)
        {
            vecPSWD[4 * idxID + j] = PSWD[j];
        }
        //printf("Se cambio la contraseña exitosamente");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_string("Clave cambiada");
        accessState = 3;
        printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n",temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
        keyPressed = 255;
    }
    else
    {
        //printf("No se cambio la contraseña");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_string("Clave no cambiada");
        accessState = 4;
        printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n",temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
        keyPressed = 255;
    }

    gKeyCnt = 0;
    hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    // led_off(GREEN_LED);
    // led_off(RED_LED);
    // led_on(YELLOW_LED);
    changePas = false;
}