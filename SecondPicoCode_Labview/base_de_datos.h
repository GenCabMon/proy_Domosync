#ifndef BASEDEDATOS_H
#define BASEDEDATOS_H

// Librerías

#include <stdint.h>
#include "pico/stdlib.h"

// vector con contraseñas
extern uint8_t vecPSWD[];

// vector con usuarios
extern uint8_t vecIDs[];

// vector que almacenara los 10 digitos ingresados por el usuario en hexadecimal
extern uint8_t hKeys[10];

extern uint8_t missCNT[10];

extern uint16_t blockIDs;

#endif