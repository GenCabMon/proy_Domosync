#ifndef MEASURELIBS_H
#define MEASURELIBS_H

#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define SAMPLES 5120
#define TAMANO_VENTANA 64 // 
#define MAX_SIZE 81 // Tamaño máximo de las señales (80 + 1 para bordes)
#define INF 1e30f     // Un valor arbitrariamente grande como "infinito"
#define PI 3.141592653589793

/* Función FFT */
void fft(int N, float real[], float imag[]);

/* Cálculo de magnitud */
void calculate_magnitude(int N, float real[], float imag[], float mag[]);

void graficar_amplitud_promedio_frecuencia(float *array, float frecuencia_muestreo, int tamano_ventana, float *amplitudes_promedio, float *indices_tiempo);

float calcular_norma(const float* arr, int size);

void calcular_correlacion_cruzada(const float* x, const float* y, int size);

float calcular_maximo(float* vector, int length, int* index_max);

float dtw(float *s1, int n, float *s2, int m);

#endif // MEASURELIBS_H