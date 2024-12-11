#ifndef MEASURELIBS_H
#define MEASURELIBS_H

/**
 * @file measurelibs.h
 * @brief Biblioteca para procesar señales, incluyendo FFT, correlación cruzada y DTW.
 * 
 * Esta biblioteca contiene funciones para realizar análisis de señales, como la Transformada Rápida de Fourier (FFT),
 * cálculo de correlación cruzada y Dynamic Time Warping (DTW), entre otras.
 * Proporciona herramientas esenciales para trabajar con señales discretas en el dominio de tiempo y frecuencia.
 */

#include "pico/stdlib.h" /**< Librería principal del SDK de Raspberry Pi Pico */
#include <stdint.h> /**< Definiciones de tipos de datos enteros con tamaño fijo */
#include <stdio.h> /**< Funciones para entrada y salida estándar */
#include <math.h> /**< Funciones matemáticas estándar como cos, sin, sqrt, etc. */

/**
 * @def SAMPLES
 * @brief Número total de muestras de la señal.
 */
#define SAMPLES 5120

/**
 * @def TAMANO_VENTANA
 * @brief Tamaño de cada ventana utilizada en el análisis de señales.
 */
#define TAMANO_VENTANA 64

/**
 * @def MAX_SIZE
 * @brief Tamaño máximo de las señales utilizadas en DTW y otros cálculos.
 */
#define MAX_SIZE 81 // Tamaño máximo de las señales (80 + 1 para bordes)

/**
 * @def INF
 * @brief Representa un valor muy grande, utilizado como "infinito" en cálculos como la DWT.
 */
#define INF 1e30f     // Un valor arbitrariamente grande como "infinito"

/**
 * @def PI
 * @brief Valor de la constante pi.
 */
#define PI 3.141592653589793

/**
 * @brief Implementa la Transformada Rápida de Fourier (FFT).
 * 
 * @param N Número de puntos de la FFT (debe ser potencia de 2).
 * @param real Array de entrada con la parte real de los datos.
 * @param imag Array de entrada con la parte imaginaria de los datos (inicialmente 0).
 */
void fft(int N, float real[], float imag[]);

/**
 * @brief Calcula la magnitud de una señal compleja a partir de sus componentes reales e imaginarias.
 * 
 * @param N Número de puntos en los arrays.
 * @param real Array de entrada con la parte real de los datos.
 * @param imag Array de entrada con la parte imaginaria de los datos.
 * @param mag Array de salida donde se almacenan las magnitudes calculadas.
 */
void calculate_magnitude(int N, float real[], float imag[], float mag[]);

/**
 * @brief Procesa una señal dividiéndola en ventanas, calcula la FFT para cada ventana 
 * y almacena las amplitudes promedio y los índices de tiempo.
 * 
 * @param array Array de entrada con la señal a procesar.
 * @param frecuencia_muestreo Frecuencia de muestreo de la señal.
 * @param tamano_ventana Tamaño de cada ventana para el procesamiento.
 * @param amplitudes_promedio Array de salida con las amplitudes promedio por ventana.
 * @param indices_tiempo Array de salida con los índices de tiempo correspondientes.
 */
void graficar_amplitud_promedio_frecuencia(float *array, float frecuencia_muestreo, int tamano_ventana, float *amplitudes_promedio, float *indices_tiempo);

/**
 * @brief Calcula la norma euclidiana de un vector.
 * 
 * @param arr Array de entrada cuyos valores serán utilizados.
 * @param size Tamaño del array.
 * @return Norma euclidiana del vector.
 */
float calcular_norma(const float* arr, int size);

/**
 * @brief Calcula la correlación cruzada normalizada entre dos señales.
 * 
 * @param x Primera señal de entrada.
 * @param y Segunda señal de entrada.
 * @param size Tamaño de las señales.
 */
void calcular_correlacion_cruzada(const float* x, const float* y, int size);

/**
 * @brief Encuentra el valor máximo en un vector y su índice correspondiente.
 * 
 * @param vector Array de entrada.
 * @param length Longitud del array.
 * @param index_max Puntero al índice donde se almacenará el índice del valor máximo.
 * @return Valor máximo del vector.
 */
float calcular_maximo(float* vector, int length, int* index_max);

/**
 * @brief Calcula la distancia entre dos secuencias(principalmente iguales) usando Dynamic Time Warping (DTW).
 * 
 * @param s1 Primera secuencia.
 * @param n Longitud de la primera secuencia.
 * @param s2 Segunda secuencia.
 * @param m Longitud de la segunda secuencia.
 * @return Distancia DTW entre las secuencias.
 */
float dtw(float *s1, int n, float *s2, int m);

#endif // MEASURELIBS_H