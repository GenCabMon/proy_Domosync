/**
 * @file access_sys.c
 * @version 1.0
 * @date 2024-10-07
 * @author Leyder Homero Marcillo Mera y Nelson Mauricio García López
 * @title Sistema de acceso
 * @brief El sistema de control de acceso cuenta con una base de datos de usuarios, donde cada usuario tiene un
 * identificador de ID y una contraseña asociada de 4 dígitos. Para otorgar acceso al usuario,
 * este deberá ingresar un ID de 6 dígitos y una contraseña de 4 dígitos. El sistema buscará en su base de datos el ID,
 * y si lo encuentra, comparará la contraseña ingresada con la contraseña del usuario. Si el ID se encuentra en la base
 * de datos y la contraseña coincide con la contraseña del usuario, entonces el sistema otorga el acceso.
 * La concesión del acceso se indicará encendiendo un LED verde durante 10 segundos. Si el ID del usuario no existe o
 * la contraseña es incorrecta, este evento se indicará encendiendo un LED rojo durante 3 segundos. En cualquier caso,
 * el sistema siempre debe recibir tanto el ID del usuario como su contraseña antes de indicar un intento de acceso
 * fallido. Si un usuario con ID ingresa la contraseña incorrectamente más de 3 veces consecutivas, no necesariamente
 * simultáneamente, el usuario es bloqueado permanentemente del sistema.
 * Otras características del sistema: El proceso de verificación de identidad tiene un tiempo máximo de 10 segundos para
 * ingresar tanto el DNI del usuario como su respectiva contraseña. Si el usuario no logra ingresar la información
 * requerida dentro de este tiempo, el sistema regresará al estado inicial, señalando con un LED rojo que el proceso
 * falló. Tanto el DNI como la contraseña deben ingresarse sin errores. No se pueden borrar dígitos. En caso de error, el
 * usuario debe terminar de ingresar el total de 10 dígitos requeridos (6 DNI + 4 contraseña) o esperar a que pase el
 * tiempo máximo de ingreso y el sistema regrese al estado inicial.
 * Un LED amarillo encendido de manera continua indicará que el sistema está listo para recibir el DNI del usuario.
 * Cuando el usuario presiona el primer dígito de su DNI, el LED se apaga, indicando que se inició el proceso de
 * verificación de identidad. Cuando el usuario ingresa los seis dígitos del DNI, el LED amarillo comienza a parpadear
 * (encendido y apagado) a una frecuencia de 0.5Hz hasta que el usuario ingresa el cuarto dígito de la contraseña.
 * En ese momento, el LED amarillo se apaga y solo se vuelve a encender cuando finaliza la señalización de contraseña
 * correcta (LED verde - 10 * seg) o contraseña incorrecta (LED rojo - 3 seg).
 * El sistema debe tener una base de datos de al menos 10 usuarios diferentes con sus respectivas contraseñas.
 * */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/adc.h"
#include "lcd_i2c.h"

// Constantes para control de servomotor
#define BUTTON 17
#define Servo_PIN 16

#define ROTATE_0 1000 // Rotate to 0° position
#define ROTATE_180 2000

#define PWM_DIV_INTEGER 125
#define PWM_DIV_FRAC 0
#define PWM_TOP_VALUE 19999

#define MAX_DUTY_CYCLE 0.1
#define MIN_DUTY_CYCLE 0.05
#define DEBOUNCE_TIME_US 5000000 // Tiempo de anti-rebote en microsegundos (500 ms)

volatile int servo_angle = 0;
volatile uint64_t last_interrupt_time_LDR = 0; // Marca de tiempo de la última interrupción
volatile uint64_t last_interrupt_time_MD = 0;  // Marca de tiempo de la última interrupción
volatile bool enable_timer_servo = false;
volatile uint64_t last_interrupt_time = 0;

// Constantes para sistema de acceso matriz
#define YELLOW_LED 11
#define GREEN_LED 12
#define RED_LED 13
#define PIN_IR 19
#define PIN_LDR 18
#define PIN_2_applauses 21
#define PIN_3_applauses 20
const uint32_t mask_flags = (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21);

// Constantes del controlador PID, sensor de temperatura
#define KP 8         // Ganancia proporcional
#define KI 0.3        // Ganancia integral
#define KD 0.1        // Ganancia derivativa
#define SETPOINT 26.0 // Temperatura deseada en °C
#define PIN_PWM 10
#define ADC_CLKDIV 47999
#define AMP_GAIN 5
#define ADC_VREF 3.3
#define ADC_RESOL 4096

// Variables globales para el sensor de temperatura
float integral = 0;
float last_error = 0;
volatile uint16_t counter_fifo = 0;
volatile uint16_t adc_raw;

/**
 * @brief Union para controlar el estado de las teclas interrupciones leds y estado de interrupciones
 */

float duty_cycle = 0;
struct repeating_timer timer; // timer para alarma de toggle led amarillo
struct repeating_timer timer_servo;

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

volatile myFlags_t gFlags; //= {false, false, false, false, false, false, false, false, false, false, false, false};

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
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 3 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 4 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 5 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 6 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 7 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, // User 8 with password 654321
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6  // User 9 with password 654321
};

// vector que almacenara los 10 digitos ingresados por el usuario en hexadecimal
uint8_t hKeys[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t missCNT[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint16_t blockIDs = 0x0000;

volatile uint8_t gKeyCnt = 0;
volatile uint8_t gSeqCnt = 0;
volatile bool gDZero = false;
volatile uint32_t gKeyCap;

volatile bool timer_fired = false; // bandera indica si el usuario ingreso los 10 digitos antes de los 10 segundos
// true= ya pasaron los 10 segundos

int8_t idxID;
int8_t IschangeP = 0;
int8_t IsShow = 0;
int8_t IsnowP = 0;
int8_t IsnowP_2 = 0;

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

void set_servo_angle(uint PWM_GPIO, uint degree)
{
    const uint count_top = PWM_TOP_VALUE;
    float duty_cycle = (float)(MIN_DUTY_CYCLE + ((degree + 90) / 180) * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE));
    pwm_set_gpio_level(PWM_GPIO, (uint16_t)(duty_cycle * (count_top + 1)));

    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
    // printf("*** PWM channel: %d ", pwm_get_counter(sliceNum));
}

/**
 * @brief captura la clave de los 10 digitos y los desplaza hacia la derecha
 *
 * @param key letra capturada con el teclado ya decifrada
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
 * @brief decifra la tecla asociada a la interrupcion asociandola a alguno de los casos posibles
 *
 * @param key letra capturada con la interrupcion
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
 * @brief Chequea si el usuario esta en la base de datos
 * @param vecID arreglo que contiene los IDs de los usuarios en la base de datos
 * @param ID ID capturado con el teclado
 * @return -1 si no se encontro en la base de datos
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
            printf("Se encontró usuario con ID %d\n", i);
            return i;
        }
    }
    return -1;
}
bool changePas = false; // bandera para cambiar la dinamica de la captura de los digitos solo captura 4 para la nueva
// contrasena

/**
 * @brief Chequeo de la contrasena asociada al mismo ID y verifica si la contrasena es FFFF se cambia la contrasena
 * del susuario
 * @param idxID ID asociado a la contrasena en cuestion
 * @param vecPSWD arreglo que contiene las contrasenas en la base de datos
 * @param PSWD contrasena capturada por el teclado
 * @return true si la contrasena coincide con el usuario asociado en la base de datos
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
        printf("Ingrese su ID y la actual contraseña\n");
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
        printf("Reconocido");
        IsnowP = 1;
        return true;
    }

    return true;
}

/**
 * @brief función para cambiar la contraseña de un usuario
 * @param idxID ID asociado a la contraseña en cuestión
 * @param vecPSWD arreglo que contiene las contrasenas en la base de datos
 * @param PSWD contraseña capturada por el teclado
 * @return true si la contraseña coincide con el usuario asociado en la base de datos
 */

void ChangePSW(int8_t idxID, uint8_t *vecPSWD, uint8_t *PSWD)
{
    int cont_F = 0;
    for (int j = 0; j < 4; j++)
    {
        if (PSWD[j] == 0xF)
        {
            printf("entro");
            cont_F++;
        }
    }
    printf("CONTF ES: %d\n", cont_F);
    if (cont_F < 4)
    {
        for (int j = 0; j < 4; j++)
        {
            vecPSWD[4 * idxID + j] = PSWD[j];
        }
        printf("Se cambio la contraseña exitosamente");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_string("Clave cambiada");
    }
    else
    {
        printf("No se cambio la contraseña");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_string("Clave no cambiada");
    }

    gKeyCnt = 0;
    hKeys[10] = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    // led_off(GREEN_LED);
    // led_off(RED_LED);
    // led_on(YELLOW_LED);
    changePas = false;
}

/**
 * @brief alarma que se activa a los 10 segundos indicando el tiempo se acabo y encinde el led rojo
 * @param id Es un identificador único de la alarma que disparó la función de callback.
 * @param user_data Es un puntero genérico que se pasa a la función de callback para almacenar datos
 * personalizados del usuario.
 * @return 0, puede ser modificado en un futuro
 */

int64_t alarm_callback_teclado(alarm_id_t id, __unused void *user_data)
{
    if (timer_fired)
    {
        printf("time out alarm 422\n");
        // led_on(RED_LED);
        gFlags.B.timeOut = true;
        timer_fired = false;
        
    }
    // Can return a value here in us to fire in the future
    return 0;
}

bool timer_toggle = false; // Bandera para saber cuando debo cancelar el repeating_timer_ms

/**
 * @brief timer para titilar el led amarillo
 * @param t puntero a la estructura para controlar el led amarillo
 */

bool repeating_timer_callback(struct repeating_timer *t)
{
    if (t == &timer)
    {
        // led_toggle(YELLOW_LED);
    }
    else if (t == &timer_servo)
    {
        enable_timer_servo = !enable_timer_servo;
    }
    return true;
}

/**
 * @brief Interrupciones del pwm
 * Slice 0 Se usa para generar una secuencia de filas, para un teclado matricial.
 * Slice 1 Se usa para el temporizador de antirrebote (debouncing).
 * Manejo de errores: Un bloque default asegura que cualquier interrupción inesperada sea capturada
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
 * @brief Se inicializa y  se configura el pwm
 * @param slice número del slice del pwm
 * @param milis tiempo en milisegundos que se desea
 * @param enable configurar automaticamente o manual
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
 * @brief inicializa el teclado, los gpios y las interrupciones asociadas
 *
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

// ISR para manejar flancos de subida y bajada
void gpio_callback_LDR(uint gpio, uint32_t events)
{

    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Ignorar interrupciones si están dentro del tiempo de anti-rebote
    if (current_time - last_interrupt_time_LDR < DEBOUNCE_TIME_US)
    {
        return;
    }

    last_interrupt_time_LDR = current_time; // Actualizar el tiempo de la última interrupción

    printf("GPIO %d, event %d\n", gpio, events);
    if (gpio == BUTTON)
    {
        if (events & GPIO_IRQ_EDGE_FALL)
        { // Flanco de subida (LOW -> HIGH)
            // Mover 90 grados el servo con PWM
            printf("Rise\n");

            // Alternar entre 0° y 90°
            if (servo_angle == 0)
            {
                servo_angle = 90;
            }
            else
            {
                servo_angle = 0;
            }

            // Establecer el nuevo ángulo del servo
            set_servo_angle(Servo_PIN, servo_angle);
        }
    }
    gpio_acknowledge_irq(gpio, events);
}

bool open = false;
void close()
{
    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Se cierra la puerta principal pasado el tiempo si esta esta abierta
    if ((current_time - last_interrupt_time > DEBOUNCE_TIME_US) && open)
    {
        printf("Ce cierra la puera \n "); 
        set_servo_angle(Servo_PIN, 0);
        open = false;
    }
    
}
//
/**
 * @brief Función principal del programa.
 *
 * Inicializa los componentes, asigna una interrupcion exclusiva al pwm, asigna prioridad a las interrupciones del pwm
 * contiene el ciclo que realiza todo el sistema de acceso, desde que se comienzan a recibir los datos del teclado, hasta que
 * se encienden los leds segun el caso que ocurra.
 *
 */

int main()
{
    stdio_init_all();
    flags_pico_init();

    sleep_ms(5000);
    printf("Hola!!!");

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
    // Initialize the GPIO input pin
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN); // Set the direction as input
    gpio_pull_up(BUTTON);          // Enable pull-up

    // Initialize the PWM pin
    project_pwm_init(Servo_PIN);
    set_servo_angle(Servo_PIN, servo_angle); // Set the initial angle to 0°

    // gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_LDR);
    gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, gpio_callback);

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
                printf("%X,%X\n", KeyData, keyd);
                if (keyd != 0xFF)
                {
                    char keyd_str[3]; // Buffer para almacenar la cadena
                    sprintf(keyd_str, "%02X", keyd); // Convertir a cadena en formato hexadecimal
                    lcd_set_cursor(3, 18);
                    lcd_string(keyd_str);
                    printf("capturo tecla: %X \n", keyd);
                    insertKey(keyd);
                    if (!IsShow && changePas)
                    { // si ya se capturaron 4 digitos y se queria cambiar contrasena se llama la funcion para cambiar la contrasena asociada al ultimo usuario ingresago
                        printf("verificacion de banderas \n");
                        IschangeP = 1;
                        IsShow = 1;
                    }
                }
                gKeyCnt++;
                if (gKeyCnt == 1 && !changePas)
                {
                    // led_off(YELLOW_LED);
                    printf("inicio conteo\n");
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
                    printf("conteo llego a 10\n");
                    // led_off(YELLOW_LED);
                    printf("hKeys: ");
                    for (int i = 0; i < 10; i++)
                    {
                        printf("0x%02X ", hKeys[i]);
                    }
                    printf("\n");
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
                                open = true;  // la puerta esta abierta
                                last_interrupt_time = time_us_64();

                                // sleep_ms(1000);
                                // set_servo_angle(Servo_PIN, 0);

                                gFlags.B.greenLed = true;
                                missCNT[idxID] = 0;
                            }
                            else
                            {
                                printf("usuario ya bloqueado \n "); // Si todo marchaba bien pero el usuario ya esta bloqueado
                                // led_on(RED_LED);
                                gFlags.B.timeOut = true;
                            }
                            }
                            
                        }
                        else if (IschangeP && OK)
                        {
                            if (IsnowP && !IsnowP_2)
                            {
                                printf("Ingrese ahora el usuario y la nueva contraseña \n");
                                lcd_clear();
                                lcd_set_cursor(0,0);
                                lcd_string("Ingrese su ID");
                                lcd_set_cursor(1,0);
                                lcd_string("y su clave nueva ");
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
                            printf("usuario suma %02X  bloqueos \n", missCNT[idxID]);
                            lcd_clear();
                            if (missCNT[idxID] > 3)
                            { // el usuario acumulo mas de 3 intentos no necesariamente consecutivos con contrasenas malas
                                blockIDs |= 0x0001 << idxID;
                                printf("usuario bloqueado %02X  \n", idxID); // usuario baneado del sistema
                                lcd_set_cursor(0, 0);
                                lcd_string("Usuario bloqueado");
                            }
                            timer_fired = true;
                            printf("Acceso denegado\n");
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
                        printf("Acceso denegado\n");
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
                set_servo_angle(Servo_PIN, 90);
                printf("Acceso concedido\n");
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_string("Acceso concedido");
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
                printf("Time out \n");
               // lcd_clear();
                //lcd_set_cursor(0, 0);
                //lcd_string("Acceso denegado");
                if (timer_fired)
                {
                    lcd_clear();
                    lcd_set_cursor(0, 0);
                    lcd_string("Acceso denegado");
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
                float temperature = (((float)adc_raw) * ADC_VREF / (ADC_RESOL * AMP_GAIN)) * 100;
                float error = temperature - SETPOINT;
                float duty_cycle = PID_controller(error);

                // Imprimir temperatura y duty cycle al monitor serial
                //printf("Temperatura: %.2f °C. Ciclo de dureza: %.5f. Error: %.5f\n", temperature, duty_cycle, error);

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
            if (!enable_timer_servo)
            {
                cancel_repeating_timer(&timer_servo);
                gpio_put(BUTTON, false);
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