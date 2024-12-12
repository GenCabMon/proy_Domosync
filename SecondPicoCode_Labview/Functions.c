#include "Functions.h"
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

int integral = 0;                 /**< Valor integral acumulado para el controlador PID. */
float last_error = 0;               /**< Último error registrado por el controlador PID. */
volatile uint64_t last_interrupt_time = 0;     /**< Marca de tiempo de la última interrupción general. */
bool open = false;         /**< Bandera para indicar si el sistema está abierto. */


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


void close()
{
    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Se cierra la puerta principal pasado el tiempo si esta esta abierta
    if ((current_time - last_interrupt_time > DEBOUNCE_TIME_US) && open)
    {
        // printf("Se cierra la puerta \n ");
        set_servo_angle(Servo_PIN, 0);
        open = false;
    }
}