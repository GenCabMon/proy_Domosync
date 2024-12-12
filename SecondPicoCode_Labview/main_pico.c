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

#include <stdint.h>       /**< Tipos de datos enteros estándar. */
#include <stdbool.h>      /**< Tipos de datos booleanos estándar. */
#include <stdio.h>        /**< Funciones estándar de entrada/salida. */
#include <math.h>         /**< Funciones matemáticas estándar. */
#include "pico/stdlib.h"  /**< Biblioteca estándar de Raspberry Pi Pico. */
#include "hardware/timer.h" /**< Funciones para control de temporizadores de hardware. */
#include "hardware/pwm.h"  /**< Funciones para control de PWM de hardware. */
#include "hardware/irq.h"  /**< Manejo de interrupciones de hardware. */
#include "hardware/gpio.h" /**< Funciones para control de pines GPIO de hardware. */
#include "hardware/sync.h" /**< Funciones de sincronización de hardware. */
#include "hardware/adc.h"  /**< Control de conversión ADC en hardware. */
#include "lcd_i2c.h"       /**< Biblioteca para control de LCD mediante comunicación I2C. */
#include "base_de_datos.h" /**< Arrays de datos para análisis de ingreso de personas a la casa. */
/** @def Servo_PIN
 *  @brief Pin GPIO para el control del servomotor.
 */
#define Servo_PIN 16

/** @def ROTATE_0
 *  @brief Ciclo de trabajo PWM para rotar el servomotor a 0°.
 */
#define ROTATE_0 1000

/** @def ROTATE_180
 *  @brief Ciclo de trabajo PWM para rotar el servomotor a 180°.
 */
#define ROTATE_180 2000

/** @def PWM_DIV_INTEGER
 *  @brief Divisor entero del reloj para el módulo PWM.
 */
#define PWM_DIV_INTEGER 125

/** @def PWM_DIV_FRAC
 *  @brief Parte fraccionaria del divisor del reloj PWM.
 */
#define PWM_DIV_FRAC 0

/** @def PWM_TOP_VALUE
 *  @brief Valor máximo del temporizador PWM.
 */
#define PWM_TOP_VALUE 19999

/** @def MAX_DUTY_CYCLE
 *  @brief Máximo ciclo de trabajo permitido (10%).
 */
#define MAX_DUTY_CYCLE 0.1

/** @def MIN_DUTY_CYCLE
 *  @brief Mínimo ciclo de trabajo permitido (5%).
 */
#define MIN_DUTY_CYCLE 0.05

/** @def DEBOUNCE_TIME_US
 *  @brief Tiempo de anti-rebote para el botón, en microsegundos.
 */
#define DEBOUNCE_TIME_US 3000000 // Tiempo de anti-rebote en microsegundos (300 ms)

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

/** @def KP
 *  @brief Constante proporcional del controlador PID.
 */
#define KP 10         // Ganancia proporcional

/** @def KI
 *  @brief Constante integral del controlador PID.
 */
#define KI 0.3        // Ganancia integral

/** @def KD
 *  @brief Constante derivativa del controlador PID.
 */
#define KD 0.1        // Ganancia derivativa

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


volatile int servo_angle = 0; /**< Ángulo actual del servomotor. */
volatile uint64_t last_interrupt_time_LDR = 0; /**< Marca de tiempo de la última interrupción del sensor LDR. */
volatile uint64_t last_interrupt_time_MD = 0;  /**< Marca de tiempo de la última interrupción de la puerta principal. */
volatile uint64_t last_interrupt_time = 0; /**< Marca de tiempo de la última interrupción general. */

uint8_t accessState = 0; /**< Estado actual del sistema de acceso. */
float temperature = 0; /**< Temperatura medida por el sensor en grados Celsius. */
float duty_cycle = 0; /**< Ciclo de trabajo actual del PWM. */
uint8_t keyPressed = 255; /**< Última tecla presionada (0x0 a 0xF para teclas, 255 para ninguna tecla). */


const uint32_t mask_flags = (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21); /**< Máscara de bits para pines relacionados con sensores. */

float integral = 0; /**< Valor integral acumulado para el controlador PID. */
float last_error = 0; /**< Último error registrado por el controlador PID. */
volatile uint16_t counter_fifo = 0; /**< Contador de muestras almacenadas en el FIFO del ADC. */
volatile uint16_t adc_raw; /**< Valor bruto del ADC leído. */


struct repeating_timer timer; /**< Temporizador utilizado para la alarma. */

volatile myFlags_t gFlags; /**< Estructura de banderas globales. */

volatile uint8_t gKeyCnt = 0; /**< Contador de teclas presionadas. */
volatile uint8_t gSeqCnt = 0; /**< Contador de secuencia del sistema de acceso. */
volatile bool gDZero = false; /**< Bandera para indicar si el sistema está en el estado de valor 0. */
volatile uint32_t gKeyCap; /**< Captura de valor clave del sistema. */

volatile bool timer_fired = false; /**< Bandera para indicar si pasaron 10 segundos sin ingresar una clave. */

int8_t idxID; /**< Índice de ID actual para la verificación de clave. */
int8_t IschangeP = 0; /**< Estado de cambio de clave (0: no, 1: sí). */
int8_t IsShow = 0; /**< Indicador de estado de visualización (0: no mostrar, 1: mostrar). */
int8_t IsnowP = 0; /**< Estado de clave actual del usuario. */
int8_t IsnowP_2 = 0; /**< Estado de la segunda clave ingresada del usuario. */

bool changePas = false; /**< Bandera para cambiar la dinámica de la captura de dígitos, permitiendo capturar solo 4 para la nueva contraseña. */
bool timer_toggle = false; /**< Bandera para indicar cuándo se debe cancelar el `repeating_timer_ms`. */
bool open = false;/**< Bandera para indicar si el sistema está abierto. */


/**
 * @brief Inicializa el PWM en el pin GPIO especificado.
 *
 * Configura el pin GPIO para usar PWM, ajusta el divisor de reloj y establece el valor tope.
 *
 * @param PWM_GPIO Pin GPIO que se utilizará para el PWM.
 */
void project_pwm_init(uint PWM_GPIO)
{
    gpio_init(PWM_GPIO);
    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_DIV_INTEGER);
    pwm_config_set_wrap(&cfg, PWM_TOP_VALUE);
    pwm_init(sliceNum, &cfg, true);
}

/**
 * @brief Configura el ángulo del servo en el pin GPIO especificado.
 *
 * Calcula el ciclo de trabajo en función del ángulo proporcionado.
 *
 * @param PWM_GPIO Pin GPIO asociado al servo.
 * @param degree Ángulo del servo en grados (0 o 90).
 */
void set_servo_angle(uint PWM_GPIO, uint degree)
{
    const uint count_top = PWM_TOP_VALUE;
    float duty_cycle = (float)(MIN_DUTY_CYCLE + ((degree + 90) / 180) * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE));
    pwm_set_gpio_level(PWM_GPIO, (uint16_t)(duty_cycle * (count_top + 1)));

    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
}

/**
 * @brief Inserta una nueva tecla en la cola de teclas desplazando las existentes.
 *
 * @param key Tecla a insertar en la cola.
 */
void insertKey(uint8_t key)
{
    for (int i = 9; i > 0; i--)
    {
        hKeys[i] = hKeys[i - 1];
    }
    hKeys[0] = key;
}

/**
 * @brief Decodifica el valor del teclado a un valor correspondiente según una tabla predefinida.
 *
 * @param keyc Código de la tecla capturado.
 * @return uint8_t Valor decodificado de la tecla o 0xFF si no coincide.
 */
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

/**
 * @brief Verifica si un ID específico coincide con uno en la lista de IDs.
 *
 * @param vecID Vector de IDs registrados.
 * @param ID ID a verificar.
 * @return int8_t Índice del ID en la lista o -1 si no se encuentra.
 */
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
        gFlags.B.greenLed = false;
        
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

/**
 * @brief Cambia la contraseña de un usuario específico.
 *
 * @param idxID Índice del ID del usuario.
 * @param vecPSWD Vector de contraseñas registradas.
 * @param PSWD Nueva contraseña a establecer.
 */
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
        //printf("time out alarm 422\n");
        // led_on(RED_LED);
        gFlags.B.timeOut = true;
        timer_fired = false;
        
    }
    // Can return a value here in us to fire in the future
    return 0;
}

/**
 * @brief Maneja los eventos de un temporizador repetitivo.
 *
 * Alterna el estado del temporizador servo el evento corresponda.
 *
 * @param t Puntero a la estructura del temporizador.
 * @return true Para continuar el temporizador.
 */
bool repeating_timer_callback(struct repeating_timer *t)
{
    if (t == &timer)
    {
        // led_toggle(YELLOW_LED);
    }
    return true;
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
 * @brief Inicializa el PWM como temporizador periódico (PIT).
 *
 * Configura un slice del PWM para generar interrupciones periódicas.
 *
 * @param slice Slice del PWM a configurar.
 * @param milis Período en milisegundos.
 * @param enable Habilita o deshabilita el slice.
 */
void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable)
{
    assert(milis <= 262); ///< PWM can manage interrupt periods greater than 262 milis
    float prescaler = (float)SYS_CLK_KHZ / 500;
    assert(prescaler < 256); ///< the integer part of the clock divider can be greater than 255
    uint32_t wrap = 500000 * milis / 2000;
    assert(wrap < ((1UL << 17) - 1));
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_phase_correct(&cfg, true);
    pwm_config_set_clkdiv(&cfg, prescaler);
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&cfg, wrap);
    pwm_set_irq_enabled(slice, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    pwm_init(slice, &cfg, enable);
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
 * @brief Controlador PID que calcula la salida basada en el error actual del sensor de temperatura LM35.
 *
 * Calcula los términos proporcional, integral y derivativo para ajustar la salida.
 *
 * @param error Diferencia entre el valor deseado y el actual.
 * @return float Salida ajustada dentro del rango [0, 100].
 */
float PID_controller(float error)
{
    integral += error; // Calcular término integral
    if (integral > 200)
        integral = 200;
    float derivative = error - last_error; // Calcular término derivativo
    float output = KP * error + KI * integral + KD * derivative;

    // Limitar la salida a [0, 100]
    if (output > 100)
        output = 100;
    if (output < 0)
        output = 0;

    last_error = error; // Actualizar el error anterior
    // output = 100;
    return output;
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
 * @brief Cierra la puerta principal si ha pasado el tiempo de rebote y está abierta.
 *
 * Comprueba el tiempo transcurrido desde la última interrupción y ajusta el servo
 * para cerrar la puerta si está abierta.
 */
void close()
{
    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Se cierra la puerta principal pasado el tiempo si esta esta abierta
    if ((current_time - last_interrupt_time > DEBOUNCE_TIME_US) && open)
    {
        //printf("Se cierra la puerta \n "); 
        set_servo_angle(Servo_PIN, 0);
        open = false;
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
    //printf("Hola!!!");

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
        if(open){
            close();
        }
        while (gFlags.W)
        {
            uint64_t current_time = time_us_64();
            if (gFlags.B.keyFlag)
            {

                // pwm_set_gpio_level(PIN_PWM, duty_cycle * 65535 / 100); // 100% = 65535
                uint32_t KeyData = (gKeyCap >> 2) & 0x000000FF;
                uint8_t keyd = keyDecode(KeyData);
                //printf("%X,%X\n", KeyData, keyd);
                if (keyd != 0xFF)
                {
                    keyPressed = keyd;
                    char keyd_str[3]; // Buffer para almacenar la cadena
                    sprintf(keyd_str, "%02X", keyd); // Convertir a cadena en formato hexadecimal
                    lcd_set_cursor(3, 18);
                    lcd_string(keyd_str);
                    //printf("capturo tecla: %X \n", keyd);
                    insertKey(keyd);
                    if (!IsShow && changePas)
                    { // si ya se capturaron 4 digitos y se queria cambiar contrasena se llama la funcion para cambiar la contrasena asociada al ultimo usuario ingresago
                        //printf("verificacion de banderas \n");
                        IschangeP = 1;
                        IsShow = 1;
                    }
                }
                gKeyCnt++;
                if (gKeyCnt == 1 && !changePas)
                {
                    // led_off(YELLOW_LED);
                    //printf("inicio conteo\n");
                    timer_fired = true;
                    add_alarm_in_ms(50000, alarm_callback_teclado, NULL, true);
                }
                else if (gKeyCnt == 6)
                {
                    timer_toggle = true;
                    add_repeating_timer_ms(2500, repeating_timer_callback, NULL, &timer); // aqui se llama la alarma de titilar el led amarillo
                }
                else if (gKeyCnt == 10)
                {
                    timer_fired = false;
                    if (timer_toggle)
                    {
                        bool cancelled = cancel_repeating_timer(&timer); // Se cancela la alarma de titilar led amarillo
                    }
                    //printf("conteo llego a 10\n");
                    // led_off(YELLOW_LED);
                    //printf("hKeys: ");
                    for (int i = 0; i < 10; i++)
                    {
                        //printf("0x%02X ", hKeys[i]);
                    }
                    //printf("\n");
                    idxID = checkID(vecIDs, &hKeys[4]); // chequeo ID existe en la base de datos
                    if (idxID != -1)
                    {
                        bool OK = checkPSWD(idxID, vecPSWD, hKeys, IschangeP); // si el ID existe entoncese se chequea la contrasena
                        if (OK && !IschangeP)
                        {
                            if (!changePas)
                            { 
                            //if ((!(blockIDs & (0x0001 << idxID))) && !changePas)
                            if (!(blockIDs & (0x0001 << idxID)))
                            { // si la contrasena esta correcta y el ususario no esta bloqueado entonces GREEN
                                // led_on(GREEN_LED);
                                last_interrupt_time = time_us_64();

                                // sleep_ms(1000);
                                // set_servo_angle(Servo_PIN, 0);

                                gFlags.B.greenLed = true;
                                missCNT[idxID] = 0;
                            }
                            else
                            {
                                //printf("usuario ya bloqueado \n "); // Si todo marchaba bien pero el usuario ya esta bloqueado
                                // led_on(RED_LED);
                                gFlags.B.timeOut = true;
                            }
                            }
                            
                        }
                        else if (IschangeP && OK)
                        {
                            if (IsnowP && !IsnowP_2)
                            {
                                //printf("Ingrese ahora el usuario y la nueva contraseña \n");
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
                            if (missCNT[idxID] > 3)
                            { // el usuario acumulo mas de 3 intentos no necesariamente consecutivos con contrasenas malas
                                blockIDs |= 0x0001 << idxID;
                                //printf("usuario bloqueado %02X  \n", idxID); // usuario baneado del sistema
                                lcd_set_cursor(0, 0);
                                lcd_string("Usuario bloqueado");
                                accessState = 1;
                                printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n",temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                                        gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                                keyPressed = 255;
                            }
                            timer_fired = true;
                            //printf("Acceso denegado\n");
                            // led_on(RED_LED);
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
                        // led_on(RED_LED);
                        timer_fired = true;
                        //printf("Acceso denegado\n");
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
                sleep_ms(100);
                open = true;  // la puerta esta abierta
                set_servo_angle(Servo_PIN, 90);
                //printf("Acceso concedido\n");
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_string("Acceso concedido");
                accessState = 2;
                printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n",temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                        gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                keyPressed = 255;
                gKeyCnt = 0;
                hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
                gFlags.B.greenLed = false;
            }
            if (gFlags.B.timeOut)
            {
                if (timer_toggle)
                {
                    bool cancelled = cancel_repeating_timer(&timer);
                }
                //printf("Time out \n");
                if (timer_fired)
                {
                    lcd_clear();
                    lcd_set_cursor(0, 0);
                    lcd_string("Acceso denegado");
                    accessState = 1;
                    printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n",temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                            gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);
                    keyPressed = 255;
                    //lcd_set_cursor(1, 0);
                    //lcd_string("Tiempo agotado");
                    lcd_set_cursor(1, 0);
                    lcd_string("Intente nuevamente");
                    
                }
                //lcd_set_cursor(2, 0);
                //lcd_string("Intente nuevamente");;
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
                //printf("Temperatura: %.2f °C. Ciclo de dureza: %.5f. Error: %.5f\n", temperature, duty_cycle, error);
                printf("TMP:%.2f IR:%s LDR:%s Bulb:%s Lamp:%s Acc:%u Duty:%.2f Key:%X\n",temperature, gFlags.B.isIR ? "1" : "0", gFlags.B.isLDR ? "1" : "0",
                gFlags.B.isRoom ? "1" : "0", gFlags.B.isLamp ? "1" : "0", accessState, duty_cycle, keyPressed);

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