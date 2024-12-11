#ifndef BASE_DE_DATOS_H
#define BASE_DE_DATOS_H

/**
 * @file base_de_datos.h
 * @brief Declaración de arrays de datos para el análisis de señales.
 *
 * Este archivo contiene las declaraciones externas de los arrays que almacenan los datos de las señales
 * de tres aplausos y dos aplausos, utilizados en el análisis de señales.
 * Los datos de estas señales se procesan en funciones de análisis como FFT y DTW, las cuales sirven como
 * comparacion para las muestras capturadas por el adc.
 */

/** 
 * @extern float Datos_tres_aplausos_1[5120]; 
 * @brief Array que contiene los datos de la señal de tres aplausos.
 * 
 * Este array almacena los 5120 datos correspondientes a la señal de tres aplausos,
 * que se utilizan para análisis en el dominio del tiempo y la frecuencia.
 */
extern float Datos_tres_aplausos_1[];  // Declarar como externo

/** 
 * @extern float Datos_dos_aplausos_1[5120]; 
 * @brief Array que contiene los datos de la señal de dos aplausos.
 * 
 * Este array almacena los 5120 datos correspondientes a la señal de dos aplausos,
 * utilizados para análisis y comparación con otras señales, capturadas por el adc.
 */
extern float Datos_dos_aplausos_1[];

#endif // BASE_DE_DATOS_H