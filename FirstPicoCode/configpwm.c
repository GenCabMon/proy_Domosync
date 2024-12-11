#include "configpwm.h"

volatile uint64_t last_interrupt_time = 0; /**< Marca de tiempo de la última interrupción. */

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
    printf("*** PWM channel: %d ", pwm_get_counter(sliceNum));
}

void set_debouncing()
{
    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Ignorar interrupciones si están dentro del tiempo de anti-rebote
    if (current_time - last_interrupt_time < DEBOUNCE_TIME_US)
    {
        return;
    }

    last_interrupt_time = current_time; // Actualizar el tiempo de la última interrupción
}