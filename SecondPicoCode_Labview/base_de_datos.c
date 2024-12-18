#include "base_de_datos.h"
// vector con contraseñas
uint8_t vecPSWD[] = {
    0x4, 0x3, 0x2, 0x1, // User 0 with ID 1234
    0xA, 0xB, 0xC, 0xD, // User 1 with ID DCBA
    0xE, 0xB, 0xE, 0xB, // User 2 with ID BEBE
    0x4, 0xC, 0x4, 0xC, // User 3 with ID C4C4
    0x1, 0x2, 0x3, 0x4, // User 4 with ID 4321
    0x0, 0xC, 0x5, 0xA, // User 5 with ID A5C0
    0x5, 0xA, 0x3, 0xF, // User 6 with ID F3A5
    0x2, 0x8, 0x9, 0x1, // User 7 with ID 1982
    0x7, 0x0, 0x0, 0x0, // User 8 with ID 0007
    0xE, 0x1, 0x1, 0x9  // User 9 with ID 911E
};
// vector con usuarios
uint8_t vecIDs[] = {
    0x6, 0x5, 0x4, 0x3, 0x2, 0x1, // User 0 with password 123456
    0x2, 0x2, 0x2, 0x1, 0x1, 0x1, // User 1 with password 222111
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // User 2 with password 000000
    0x4, 0x2, 0x3, 0x4, 0x5, 0x6, // User 3 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 4 with password 654321
    0x2, 0x2, 0x3, 0x4, 0x5, 0x6, // User 5 with password 654321
    0x3, 0x2, 0x3, 0x4, 0x5, 0x6, // User 6 with password 654321
    0x4, 0x2, 0x3, 0x4, 0x5, 0x6, // User 7 with password 654321
    0x5, 0x2, 0x3, 0x4, 0x5, 0x6, // User 8 with password 654321
    0x6, 0x2, 0x3, 0x4, 0x5, 0x6  // User 9 with password 654321
};

// vector que almacenara los 10 digitos ingresados por el usuario en hexadecimal
uint8_t hKeys[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t missCNT[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint16_t blockIDs = 0x0000;