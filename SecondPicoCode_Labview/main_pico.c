/**
 * @file main_pico.c
 * @version 1.0
 * @date 2024-12-12
 * @title Sistema de acceso y control de flujo de sensores
 * @brief Este archivo contiene el código fuente para el control de un sistema de acceso inteligente utilizando un microcontrolador Raspberry Pi Pico.
 *
 * Este sistema permite la gestión de accesos a través de un teclado matricial, un lector de temperatura y control de un ventilador, ademas el control de luces y un servo motor.
 * Además, se implementan diversas funcionalidades como la lectura de un sensor de temperatura, control de luces, manejo de contraseñas de usuarios y un sistema de alarmas.
 * La comunicación con dispositivos periféricos como pantallas LCD y otros módulos de hardware es realizada en este archivo.
 *
 * @section Descripción General
 * Este sistema de acceso realiza las siguientes funciones:
 * - Inicializa los módulos de entrada/salida del sistema (GPIO).
 * - Configura un ADC para leer valores del sensor de temperatura.
 * - Usa un teclado matricial 4x4 para recibir entradas de usuario.
 * - Controla un servo motor para abrir una puerta de acceso.
 * - Muestra información sobre el estado del sistema en un LCD.
 * - Utiliza un sistema de interrupciones para manejar eventos como la lectura del ADC, cambios en el teclado y el control del servo.
 *
 * @section Estructura del código
 * - `main()` es la función principal que ejecuta la inicialización de hardware, configura interrupciones y maneja el ciclo principal del sistema.
 * - Varias funciones auxiliares se encargan de tareas específicas como el control de contraseñas, la configuración del PWM para el servo y la gestión de interrupciones.
 */

#include <stdint.h>         /**< Tipos de datos enteros estándar. */
#include <stdbool.h>        /**< Tipos de datos booleanos estándar. */
#include <stdio.h>          /**< Funciones estándar de entrada/salida. */
#include <math.h>           /**< Funciones matemáticas estándar. */
#include "pico/stdlib.h"    /**< Biblioteca estándar de Raspberry Pi Pico. */
#include "hardware/timer.h" /**< Funciones para control de temporizadores de hardware. */
#include "hardware/pwm.h"   /**< Funciones para control de PWM de hardware. */
#include "hardware/irq.h"   /**< Manejo de interrupciones de hardware. */
#include "hardware/gpio.h"  /**< Funciones para control de pines GPIO de hardware. */
#include "hardware/sync.h"  /**< Funciones de sincronización de hardware. */
#include "hardware/adc.h"   /**< Control de conversión ADC en hardware. */
#include "lcd_i2c.h"        /**< Biblioteca para control de LCD mediante comunicación I2C. */
#include "base_de_datos.h"  /**< Arrays de datos para análisis de ingreso de personas a la casa. */
#include "access_system.h"
#include "Functions.h"

/** @def ROTATE_0
 *  @brief Ciclo de trabajo PWM para rotar el servomotor a 0°.
 */
#define ROTATE_0 1000

/** @def ROTATE_180
 *  @brief Ciclo de trabajo PWM para rotar el servomotor a 180°.
 */
#define ROTATE_180 2000

/** @def PWM_DIV_FRAC
 *  @brief Parte fraccionaria del divisor del reloj PWM.
 */
#define PWM_DIV_FRAC 0

/** @def YELLOW_LED
 *  @brief Pin GPIO del LED amarillo.
 */
#define YELLOW_LED 11

/** @def GREEN_LED
 *  @brief Pin GPIO del LED verde.
 */
#define GREEN_LED 12

/** @def RED_LED
 *  @brief Pin GPIO del LED rojo.
 */
#define RED_LED 13

/** @def PIN_IR
 *  @brief Pin GPIO para recibir la señal del sensor infrarrojo.
 */
#define PIN_IR 19

/** @def PIN_LDR
 *  @brief Pin GPIO para recibir la señal del sensor LDR (Light Dependent Resistor).
 */
#define PIN_LDR 18

/** @def PIN_2_applauses
 *  @brief Pin GPIO asociado a la detección de dos aplausos.
 */
#define PIN_2_applauses 21

/** @def PIN_3_applauses
 *  @brief Pin GPIO asociado a la detección de tres aplausos.
 */
#define PIN_3_applauses 20

/** @def SETPOINT
 *  @brief Temperatura deseada en grados Celsius.
 */
#define SETPOINT 26.0 // Temperatura deseada en °C

/** @def PIN_PWM
 *  @brief Pin GPIO utilizado para generar señal PWM.
 */
#define PIN_PWM 10

/** @def ADC_CLKDIV
 *  @brief Divisor de reloj para el ADC.
 */
#define ADC_CLKDIV 47999

/** @def AMP_GAIN
 *  @brief Ganancia del amplificador en el circuito ADC.
 */
#define AMP_GAIN 5

/** @def ADC_VREF
 *  @brief Referencia de voltaje del ADC en voltios para el LM35.
 */
#define ADC_VREF 3.3

/** @def ADC_RESOL
 *  @brief Resolución del ADC (valores discretos).
 */
#define ADC_RESOL 4096

volatile int servo_angle = 0;                  /**< Ángulo actual del servomotor. */
volatile uint64_t last_interrupt_time_LDR = 0; /**< Marca de tiempo de la última interrupción del sensor LDR. */
volatile uint64_t last_interrupt_time_MD = 0;  /**< Marca de tiempo de la última interrupción de la puerta principal. */

const uint32_t mask_flags = (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21); /**< Máscara de bits para pines relacionados con sensores. */

volatile uint16_t counter_fifo = 0; /**< Contador de muestras almacenadas en el FIFO del ADC. */
volatile uint16_t adc_raw;          /**< Valor bruto del ADC leído. */

volatile uint8_t gSeqCnt = 0; /**< Contador de secuencia del sistema de acceso. */
volatile bool gDZero = false; /**< Bandera para indicar si el sistema está en el estado de valor 0. */
volatile uint32_t gKeyCap;    /**< Captura de valor clave del sistema. */

int8_t idxID;         /**< Índice de ID actual para la verificación de clave. */
int8_t IschangeP = 0; /**< Estado de cambio de clave (0: no, 1: sí). */
int8_t IsShow = 0;    /**< Indicador de estado de visualización (0: no mostrar, 1: mostrar). */

bool IsprintLCD = false;

/**
 * @brief Llama al callback de alarma del teclado.
 *
 * Establece un indicador de tiempo de espera y reinicia el temporizador.
 *
 * @param id Identificador de la alarma.
 * @param user_data Datos adicionales proporcionados al callback, identificador adicional.
 * @return int64_t Tiempo en microsegundos para la próxima alarma.
 */
int64_t alarm_callback_teclado(alarm_id_t id, __unused void *user_data)
{
    if (timer_fired)
    {
        // printf("time out alarm 422\n");
        //  led_on(RED_LED);
        gFlags.B.timeOut = true;
        timer_fired = false;
    }
    // Can return a value here in us to fire in the future
    return 0;
}

/**
 * @brief Interrupción del módulo PWM para manejar diferentes eventos.
 *
 * Maneja las interrupciones según el estado del PWM y actualiza las variables globales.
 */
void pwmIRQ(void)
{
    uint32_t gpioValue;
    uint32_t keyc;
    // pwm_set_gpio_level(PIN_PWM, duty_cycle * 65535 / 100);
    switch (pwm_get_irq_status_mask())
    {
    case 0x01UL: ///< PWM slice 0 ISR used as a PIT to generate row sequence
        gSeqCnt = (gSeqCnt + 1) % 4;
        gpio_put_masked(0x0000003C, 0x00000001 << (gSeqCnt + 2));
        pwm_clear_irq(0); ///< Reconocer el 0 en el PWM IRQ
        break;
    case 0x02UL: ///< PWM slice 1 ISR utilizado como PIT para implementar el antirrebote
        gFlags.B.keyDbnc = true;
        pwm_clear_irq(1); ///< Reconocer el 1 en el PWM IRQ
        break;
    default:
        printf("Paso lo que no debería pasar en PWM IRQ\n");
        break;
    }
}

/**
 * @brief Callback para manejar interrupciones en pines GPIO específicos.
 *
 * Actualiza banderas globales en respuesta a cambios de estado de los pines GPIO.
 *
 * @param gpio Pin GPIO que generó la interrupción.
 * @param events Eventos asociados a la interrupción.
 */
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == PIN_IR && (events & GPIO_IRQ_EDGE_FALL))
    {
        gFlags.B.isIR = false;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_IR && (events & GPIO_IRQ_EDGE_RISE))
    {
        gFlags.B.isIR = true;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_LDR && (events & GPIO_IRQ_EDGE_FALL))
    {
        gFlags.B.isLDR = false;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_LDR && (events & GPIO_IRQ_EDGE_RISE))
    {
        gFlags.B.isLDR = true;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_2_applauses && (events & GPIO_IRQ_EDGE_FALL))
    {
        gFlags.B.isLamp = false;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_2_applauses && (events & GPIO_IRQ_EDGE_RISE))
    {
        gFlags.B.isLamp = true;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_3_applauses && (events & GPIO_IRQ_EDGE_FALL))
    {
        gFlags.B.isRoom = false;
        gFlags.B.isLights = true;
    }
    else if (gpio == PIN_3_applauses && (events & GPIO_IRQ_EDGE_RISE))
    {
        gFlags.B.isRoom = true;
        gFlags.B.isLights = true;
    }
    else
    {
        gKeyCap = gpio_get_all();
        gFlags.B.keyFlag = true;
        pwm_set_enabled(0, false); ///< Froze the row sequence
        pwm_set_enabled(1, true);
        gpio_set_irq_enabled(6, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(7, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(8, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(9, GPIO_IRQ_EDGE_RISE, false);
    }
    gpio_acknowledge_irq(gpio, events);
}

/**
 * @brief Inicializa las banderas de GPIO y configura las interrupciones asociadas.
 *
 * Configura las entradas con resistencia pull-down y habilita histeresis en los pines
 * relacionados con IR, LDR, y sensores de aplausos.
 */
void flags_pico_init()
{
    gpio_init_mask(mask_flags);
    gpio_set_dir_in_masked(mask_flags);
    gpio_pull_down(PIN_IR);
    gpio_pull_down(PIN_LDR);
    gpio_pull_down(PIN_2_applauses);
    gpio_pull_down(PIN_3_applauses);
    gpio_set_input_hysteresis_enabled(PIN_IR, true);
    gpio_set_input_hysteresis_enabled(PIN_LDR, true);
    gpio_set_input_hysteresis_enabled(PIN_2_applauses, true);
    gpio_set_input_hysteresis_enabled(PIN_3_applauses, true);
    gpio_set_irq_enabled_with_callback(19, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_callback); // IR_sens
    gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_callback); // LDR_sens
    gpio_set_irq_enabled_with_callback(20, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_callback); // 3_applause_sens
    gpio_set_irq_enabled_with_callback(21, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_callback); // 2_applause_sens
}

/**
 * @brief Inicializa el teclado matricial 4x4 configurando filas y columnas.
 *
 * Las filas (GPIOs 2 a 5) se configuran como salidas y las columnas (GPIOs 6 a 9)
 * como entradas con interrupciones asociadas a flancos ascendentes.
 */
void initMatrixKeyboard4x4(void)
{
    // GPIOs 5 to 2 control keyboard rows (one hot sequence)
    // GPIOS 9 to 6 control keyboard columns (GPIO IRQs)
    // Lets configure who controls the GPIO PAD
    gpio_set_function(2, GPIO_FUNC_SIO);
    gpio_set_function(3, GPIO_FUNC_SIO);
    gpio_set_function(4, GPIO_FUNC_SIO);
    gpio_set_function(5, GPIO_FUNC_SIO);
    gpio_set_function(6, GPIO_FUNC_SIO);
    gpio_set_function(7, GPIO_FUNC_SIO);
    gpio_set_function(8, GPIO_FUNC_SIO);
    gpio_set_function(9, GPIO_FUNC_SIO);

    gpio_set_dir_in_masked(0x000003C0);  // Set gpios 6 to 9 as inputs (columns)
    gpio_set_dir_out_masked(0x0000003C); // Set gpios 2 to 5 as outputs (rows)
    gpio_put_masked(0x0000003C, 0);      // Write 0 to rows

    // Asigna las interrupciones a los gpios  como salidas (columnas)
    gpio_set_irq_enabled_with_callback(6, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(7, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(8, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(9, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
}

/**
 * @brief Maneja las interrupciones del ADC.
 *
 * Incrementa un contador y obtiene el valor del FIFO del ADC cuando el contador alcanza 200.
 */
void adc_handler()
{
    counter_fifo += 1;
    if (counter_fifo == 200)
    {
        counter_fifo = 0;
        adc_raw = adc_fifo_get();
        gFlags.B.adcHandler = true;
    }
}

/**
 * @brief Función principal que gestiona el funcionamiento de un sistema de acceso y control de luces basado en un teclado, sensores y servomotor.
 *
 * La función principal configura e inicializa diversos periféricos y controladores en el sistema, incluidos ADC, PWM, teclados matriciales, y el servomotor.
 * Luego entra en un bucle infinito donde maneja el flujo de eventos como la captura de teclas, la verificación de contraseñas, el control de acceso,
 * y la gestión de las luces y otros dispositivos según el estado de los sensores y entradas del usuario.
 *
 * El proceso se realiza mediante interrupciones, temporizadores y flags que indican el estado de las operaciones.
 * La función también maneja la interacción con una pantalla LCD para mostrar información al usuario.
 *
 * @note El sistema utiliza varios timers y GPIOs para manejar las señales y eventos, incluyendo un control PID para ajustar la temperatura a un valor objetivo.
 */

int main()
{
    stdio_init_all();
    flags_pico_init();

    sleep_ms(5000);
    // printf("Hola!!!");

    lcd_init(14, 15);
    lcd_string("Iniciando...");

    //==========================Inicializacion del ADC===============================

    adc_init();
    adc_gpio_init(27);   // Pin GPIO para LM35
    adc_select_input(1); // Canal 1
    adc_set_clkdiv((float)ADC_CLKDIV);
    adc_fifo_setup(
        true,  // Habilita FIFO
        false, // No usa DMA
        4,     // Umbral de FIFO en 4
        false, // No incluir errores en FIFO
        false  // No reduce resolución a 8 bits
    );
    // Configura la interrupción
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_irq_set_enabled(true);
    adc_run(true);

    // Configurar PWM
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PIN_PWM);
    pwm_set_clkdiv(slice_num, 32.0f);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_enabled(slice_num, true);

    //===============================================================================

    initPWMasPIT(0, 2, true);
    initPWMasPIT(1, 100, false);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmIRQ); // asigna una interrupcion exclusiva al pwm
    irq_set_priority(PWM_IRQ_WRAP, 0xC0);            // Asigna prioridad a las interrupciones del pwm

    initMatrixKeyboard4x4();
    /// Init Polling base Periodic Time Base

    // led_init(YELLOW_LED);
    // led_init(RED_LED);
    // led_init(GREEN_LED);
    // gpio_put(YELLOW_LED, 1);

    //=========================Inicializacion control de servo ===========================

    // Initialize the PWM pin
    project_pwm_init(Servo_PIN);
    set_servo_angle(Servo_PIN, servo_angle); // Set the initial angle to 0°

    sleep_ms(2000);
    lcd_set_cursor(0, 0);
    lcd_string("Bienvenido al");

    lcd_set_cursor(1, 0);
    lcd_string("Apto. inteligente");

    lcd_set_cursor(2, 0);
    lcd_string("de DomoSync");

    lcd_set_cursor(2, 0);
    lcd_string("Access System: ON");

    lcd_set_cursor(3, 0);
    lcd_string("Sensory: ON");
    sleep_ms(3000);
    lcd_clear();

    lcd_set_cursor(0, 0);
    lcd_string("Luces: MainDoor OFF");

    lcd_set_cursor(1, 0);
    lcd_string("Kitchen OFF");

    lcd_set_cursor(2, 0);
    lcd_string("Bulb/Lamp OFF/OFF");

    while (1)
    {
        if (open)
        {
            close();
        }
        while (gFlags.W)
        {
            uint64_t current_time = time_us_64();
            if (gFlags.B.keyFlag)
            {

                uint32_t KeyData = (gKeyCap >> 2) & 0x000000FF;
                uint8_t keyd = keyDecode(KeyData);
                if (keyd != 0xFF)
                {
                    keyPressed = keyd;
                    char keyd_str[3];                // Buffer para almacenar la cadena
                    sprintf(keyd_str, "%02X", keyd); // Convertir a cadena en formato hexadecimal
                    lcd_set_cursor(3, 18);
                    lcd_string(keyd_str);
                    insertKey(keyd);
                    if (!IsShow && changePas)
                    { // si ya se capturaron 4 digitos y se queria cambiar contrasena se llama la funcion para cambiar la contrasena asociada al ultimo usuario ingresago
                        // printf("verificacion de banderas \n");
                        IschangeP = 1;
                        IsShow = 1;
                    }
                }
                gKeyCnt++;
                if (gKeyCnt == 1 && !changePas)
                {
                    timer_fired = true;
                    add_alarm_in_ms(50000, alarm_callback_teclado, NULL, true);
                }
                else if (gKeyCnt >= 10)
                {
                    timer_fired = false;
                    for (int i = 0; i < 10; i++)
                    {
                        // printf("0x%02X ", hKeys[i]);
                    }
                    idxID = checkID(vecIDs, &hKeys[4]); // chequeo ID existe en la base de datos
                    if (idxID != -1)
                    {
                        bool OK = checkPSWD(idxID, vecPSWD, hKeys, IschangeP); // si el ID existe entoncese se chequea la contrasena
                        if (OK && !IschangeP)
                        {
                            if (!changePas)
                            {
                                // if ((!(blockIDs & (0x0001 << idxID))) && !changePas)
                                if (!(blockIDs & (0x0001 << idxID)))
                                { // si la contrasena esta correcta y el ususario no esta bloqueado entonces GREEN
                                    last_interrupt_time = time_us_64();

                                    gFlags.B.greenLed = true;
                                    missCNT[idxID] = 0;
                                }
                                else
                                {
                                    // printf("usuario ya bloqueado \n "); // Si todo marchaba bien pero el usuario ya esta bloqueado
                                    //  led_on(RED_LED);
                                    gFlags.B.timeOut = true;
                                }
                            }
                        }
                        else if (IschangeP && OK)
                        {
                            if (IsnowP && !IsnowP_2)
                            {
                                // printf("Ingrese ahora el usuario y la nueva contraseña \n");
                                lcd_clear();
                                lcd_set_cursor(0, 0);
                                lcd_string("Ingrese su ID");
                                lcd_set_cursor(1, 0);
                                lcd_string("y su clave nueva");
                            }
                            if (IsnowP && IsnowP_2)
                            {
                                ChangePSW(idxID, vecPSWD, hKeys);
                                IschangeP = 0;
                                IsShow = 0;
                                IsnowP = 0;
                                IsnowP_2 = 0;
                                changePas = 0;
                            }
                        }
                        else
                        {
                            missCNT[idxID]++; // El usuario existe pero la contrasena esta mala (lleva 1)
                            //printf("usuario suma %02X  bloqueos \n", missCNT[idxID]);
                            lcd_clear();
                            if (missCNT[idxID] >= 3)
                            { // el usuario acumulo mas de 3 intentos no necesariamente consecutivos con contrasenas malas
                                IsprintLCD = true;
                                blockIDs |= 0x0001 << idxID;
                                accessState = 1;
                                printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n", temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                                       gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                                keyPressed = 255;
                            }
                            timer_fired = true;
                            gFlags.B.timeOut = true;
                            IschangeP = 0;
                            IsShow = 0;
                            IsnowP = 0;
                            IsnowP_2 = 0;
                            changePas = 0;
                        }
                    }
                    else
                    {
                        timer_fired = true;
                        gFlags.B.timeOut = true;
                    }
                    gKeyCnt = 0;                                                              // reinicia conteo
                    hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF); // reinciar vector que almacena 10 digitos del usuario
                    gFlags.B.timeOut = true;
                }
                gFlags.B.keyFlag = false;
            }
            if (gFlags.B.keyDbnc)
            {
                uint32_t keyc = gpio_get_all() & 0x000003C0; // Get raw gpio values
                if (gDZero)
                {
                    if (!keyc)
                    {
                        pwm_set_enabled(0, true); // Froze the row sequence
                        pwm_set_enabled(1, false);
                        gpio_set_irq_enabled(6, GPIO_IRQ_EDGE_RISE, true);
                        gpio_set_irq_enabled(7, GPIO_IRQ_EDGE_RISE, true);
                        gpio_set_irq_enabled(8, GPIO_IRQ_EDGE_RISE, true);
                        gpio_set_irq_enabled(9, GPIO_IRQ_EDGE_RISE, true);
                    }
                    gDZero = false;
                }
                else
                {
                    gDZero = true;
                }
                gFlags.B.keyDbnc = false;
            }
            if (gFlags.B.greenLed)
            {
                open = true; // la puerta esta abierta

                sleep_ms(100);
                set_servo_angle(Servo_PIN, 90);
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_string("Acceso concedido");
                accessState = 2;
                printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n", temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                       gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                keyPressed = 255;
                gKeyCnt = 0;
                hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
                gFlags.B.greenLed = false;
            }
            if (gFlags.B.timeOut)
            {
                
                if (timer_fired)
                {
                    lcd_clear();
                    lcd_set_cursor(0, 0);
                    lcd_string("Acceso denegado");
                    accessState = 1;
                    printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n", temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                           gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                    keyPressed = 255;
                    lcd_set_cursor(1, 0);
                    lcd_string("Intente nuevamente");

                    if (IsprintLCD)
                    {
                        //printf("usuario bloqueado %02X  \n", idxID); // usuario baneado del sistema
                        lcd_set_cursor(0, 0);
                        lcd_string("Usuario bloqueado");
                    }

                    IsprintLCD = false;
                }
                gKeyCnt = 0;
                hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
                gFlags.B.timeOut = false;
                timer_fired = false;
            }
            if (gFlags.B.adcHandler)
            {
                // printf("ADC: %u\n", adc_raw);
                temperature = (((float)adc_raw) * ADC_VREF / (ADC_RESOL * AMP_GAIN)) * 100;
                float error = temperature - SETPOINT;
                duty_cycle = PID_controller(error);

                // Imprimir temperatura y duty cycle al monitor serial
                // printf("Temperatura: %.2f °C. Ciclo de dureza: %.5f. Error: %.5f\n", temperature, duty_cycle, error);
                printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n", temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                 gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                 keyPressed = 255;

                // Ajustar PWM aquí usando duty_cycle
                pwm_set_gpio_level(PIN_PWM, duty_cycle * 65535 / 100); // 100% = 65535

                sleep_ms(400);
                char buffer_temp[20]; // Tamaño máximo del buffer: 20 caracteres
                // Formatear la cadena
                snprintf(buffer_temp, sizeof(buffer_temp), "%.2f Celsius", temperature);
                lcd_set_cursor(3, 0);
                lcd_string(buffer_temp);
                gFlags.B.adcHandler = 0;
            }
            if (gFlags.B.isLights)
            {
                char buffer_LDR[20];
                char buffer_IR[20];
                char buffer_Room[20];
                if (gFlags.B.isLDR)
                {
                    snprintf(buffer_LDR, sizeof(buffer_LDR), "Luces: MainDoor ON");
                }
                else
                {
                    snprintf(buffer_LDR, sizeof(buffer_LDR), "Luces: MainDoor OFF");
                }
                if (gFlags.B.isIR)
                {
                    snprintf(buffer_IR, sizeof(buffer_IR), "Kitchen ON");
                }
                else
                {
                    snprintf(buffer_IR, sizeof(buffer_IR), "Kitchen OFF");
                }
                if (!gFlags.B.isRoom && !gFlags.B.isLamp)
                {
                    snprintf(buffer_Room, sizeof(buffer_Room), "Bulb/Lamp OFF/OFF");
                }
                else if (gFlags.B.isRoom && !gFlags.B.isLamp)
                {
                    snprintf(buffer_Room, sizeof(buffer_Room), "Bulb/Lamp ON/OFF");
                }
                else if (!gFlags.B.isRoom && gFlags.B.isLamp)
                {
                    snprintf(buffer_Room, sizeof(buffer_Room), "Bulb/Lamp OFF/ON");
                }
                else if (gFlags.B.isRoom && gFlags.B.isLamp)
                {
                    snprintf(buffer_Room, sizeof(buffer_Room), "Bulb/Lamp ON/ON");
                }
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_string(buffer_LDR);
                lcd_set_cursor(1, 0);
                lcd_string(buffer_IR);
                lcd_set_cursor(2, 0);
                lcd_string(buffer_Room);
                gFlags.B.isLights = false;
            }
        }
        __wfi();
    }
}