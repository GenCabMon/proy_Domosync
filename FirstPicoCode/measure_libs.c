#include "measure_libs.h"

/* Función FFT */
void fft(int N, float real[], float imag[])
{
    int i, j, k, m, n, step;
    float tReal, tImag, uReal, uImag, wReal, wImag, angle;

    // Reorganización en orden de bits invertidos
    j = 0;
    for (i = 0; i < N; i++)
    {
        if (i < j)
        {
            // Intercambio de datos
            tReal = real[i];
            tImag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tReal;
            imag[j] = tImag;
        }
        m = N / 2;
        while (j >= m && m >= 2)
        {
            j -= m;
            m /= 2;
        }
        j += m;
    }

    // Etapas de la FFT
    for (step = 2; step <= N; step *= 2)
    {
        angle = -2.0 * PI / step;
        wReal = cos(angle);
        wImag = sin(angle);

        for (k = 0; k < N; k += step)
        {
            uReal = 1.0;
            uImag = 0.0;

            for (j = 0; j < step / 2; j++)
            {
                i = k + j;
                m = i + step / 2;

                tReal = uReal * real[m] - uImag * imag[m];
                tImag = uReal * imag[m] + uImag * real[m];

                real[m] = real[i] - tReal;
                imag[m] = imag[i] - tImag;

                real[i] += tReal;
                imag[i] += tImag;

                // Actualización del factor de rotación
                tReal = uReal * wReal - uImag * wImag;
                uImag = uReal * wImag + uImag * wReal;
                uReal = tReal;
            }
        }
    }
}

/* Cálculo de magnitud */
void calculate_magnitude(int N, float real[], float imag[], float mag[])
{
    for (int i = 0; i < N; i++)
    {
        mag[i] = sqrt(real[i] * real[i] + imag[i] * imag[i]);
    }
}

void graficar_amplitud_promedio_frecuencia(float *array, float frecuencia_muestreo, int tamano_ventana, float *amplitudes_promedio, float *indices_tiempo)
{
    // Calcular el número de ventanas
    int num_ventanas = SAMPLES/tamano_ventana;

    // Procesar cada ventana
    for (int i = 0; i < num_ventanas; i++)
    {
        // Índices para la ventana actual
        int inicio = i * tamano_ventana;
        int fin = inicio + tamano_ventana;

        // Crear arrays para la ventana actual (estáticos, de tamaño fijo)
        float ventana_real[tamano_ventana];
        float ventana_imag[tamano_ventana];

        // Copiar los datos de la ventana a los arrays de la FFT
        for (int j = 0; j < tamano_ventana; j++)
        {
            ventana_real[j] = array[inicio + j];
            ventana_imag[j] = 0.0f; // Inicializar la parte imaginaria a 0
        }

        // Calcular la FFT de la ventana
        fft(tamano_ventana, ventana_real, ventana_imag);

        // Calcular la magnitud de las frecuencias y la amplitud promedio
        float suma_magnitudes = 0.0f;
        float mag[tamano_ventana];
        calculate_magnitude(tamano_ventana, ventana_real, ventana_imag, mag);

        float Promedio = 0;
        for (int i = 0; i < tamano_ventana; i++)
        {
            Promedio+=mag[i];
        }
        amplitudes_promedio[i] = Promedio/tamano_ventana;

        // Calcular el índice de tiempo para esta ventana
        indices_tiempo[i] = (float)(inicio + tamano_ventana / 2) / frecuencia_muestreo;
    }

    printf("Indice\tMagnitud\n");
    for (int i = 0; i < SAMPLES / TAMANO_VENTANA; i++)
    {
        printf("%.5f\t\t%.2f\n", indices_tiempo[i], amplitudes_promedio[i]);
    }


}

// Función para calcular la norma euclidiana
float calcular_norma(const float* arr, int size) {
    float suma = 0.0;
    for (int i = 0; i < size; i++) {
        suma += arr[i] * arr[i];
    }
    return sqrt(suma);
}

// Función para calcular la correlación cruzada normalizada
void calcular_correlacion_cruzada(const float* x, const float* y, int size) {
    
    // Arreglo para almacenar la correlación cruzada
    int resultado_size = 2 * size - 1;
    float resultado[resultado_size];
    
    float norma_x = calcular_norma(x, size);
    float norma_y = calcular_norma(y, size);

    for (int lag = -size + 1; lag < size; lag++) {
        float suma = 0.0;

        for (int i = 0; i < size; i++) {
            int j = i + lag;
            if (j >= 0 && j < size) {
                suma += x[i] * y[j];
            }
        }

        int idx = lag + size - 1;  // Ajustar índice para el resultado
        resultado[idx] = suma / (norma_x * norma_y);
    }

    // Mostrar resultados
    for (int i = 0; i < resultado_size; i++) {
        printf("Lag %d: %f\n", i - (size - 1), resultado[i]);
    }

    int index_max;
    float max_val = calcular_maximo(resultado, resultado_size, &index_max);

    // Imprimir el valor máximo y su índice
    printf("Valor máximo: %.5f\n", max_val);
    printf("Índice del valor máximo: %d\n", index_max);

}

// Función para calcular el valor máximo de un vector de floats
float calcular_maximo(float* vector, int length, int* index_max) {
    float max_val = vector[0]; // Suponemos que el primer valor es el máximo inicialmente
    *index_max = 0; // El índice del valor máximo
    for (int i = 1; i < length; i++) {
        if (vector[i] > max_val) {
            max_val = vector[i];
            *index_max = i; // Actualizamos el índice cuando encontramos un nuevo máximo
        }
    }
    return max_val; // Devuelve el valor máximo
}

// Función para calcular DTW con float y matriz fija
float dtw(float *s1, int n, float *s2, int m) {
    float dtw_matrix[MAX_SIZE][MAX_SIZE];


    // Inicializar la matriz con valores "infinito" (FLT_MAX)
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= m; j++) {
            dtw_matrix[i][j] = INF;
        }
    }
    dtw_matrix[0][0] = 0;

    // Calcular matriz de distancias acumuladas
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            float cost = (s1[i - 1] - s2[j - 1]) * (s1[i - 1] - s2[j - 1]); // Diferencia al cuadrado
            dtw_matrix[i][j] = cost + fminf(fminf(
                dtw_matrix[i - 1][j],    // Arriba
                dtw_matrix[i][j - 1]),   // Izquierda
                dtw_matrix[i - 1][j - 1] // Diagonal
            );
        }
    }

    // Resultado final: raíz cuadrada de la suma acumulada
    return sqrtf(dtw_matrix[n][m]);
}