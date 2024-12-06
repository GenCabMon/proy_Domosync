/**
 * @file main.c
 * @brief This is a brief description of the main C file.
 *
 * Detailed description of the main C file.
 */

// Standard libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
// Include your own header files here

/**
 * @brief Main program.
 *
 * This function initializes the MCU and does an infinite cycle.
 */

#define BUTTON              16
#define Servo_PIN           15 

#define ROTATE_0            1000 //Rotate to 0° position
#define ROTATE_180          2000

#define PWM_DIV_INTEGER     125
#define PWM_DIV_FRAC        0
#define PWM_TOP_VALUE       19999

#define MAX_DUTY_CYCLE      0.1
#define MIN_DUTY_CYCLE      0.05
#define DEBOUNCE_TIME_US    500000 // Tiempo de anti-rebote en microsegundos (500 ms)


volatile int servo_angle = 0;
volatile uint64_t last_interrupt_time = 0; // Marca de tiempo de la última interrupción


void project_pwm_init(uint PWM_GPIO) {
    gpio_init(PWM_GPIO);
    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_DIV_INTEGER);
    pwm_config_set_wrap(&cfg, PWM_TOP_VALUE);
    pwm_init(sliceNum, &cfg, true);
}

void set_servo_angle(uint PWM_GPIO, uint degree) {
    const uint count_top = PWM_TOP_VALUE; 
    float duty_cycle = (float)(MIN_DUTY_CYCLE + ((degree +90)/180) * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE));
    pwm_set_gpio_level(PWM_GPIO, (uint16_t)(duty_cycle * (count_top + 1)));

    uint sliceNum = pwm_gpio_to_slice_num(PWM_GPIO);
    printf("*** PWM channel: %d ", pwm_get_counter(sliceNum));
}



// ISR para manejar flancos de subida y bajada
void gpio_callback_LDR(uint gpio, uint32_t events) {

    
    uint64_t current_time = time_us_64(); // Tiempo actual en microsegundos

    // Ignorar interrupciones si están dentro del tiempo de anti-rebote
    if (current_time - last_interrupt_time < DEBOUNCE_TIME_US) {
        return;
    }

    last_interrupt_time = current_time; // Actualizar el tiempo de la última interrupción


    printf("GPIO %d, event %d\n", gpio, events);
    if (gpio == BUTTON) {
        if (events & GPIO_IRQ_EDGE_FALL) {  // Flanco de subida (LOW -> HIGH)
            // Mover 90 grados el servo con PWM
            printf("Rise\n");

            // Alternar entre 0° y 90°
            if (servo_angle == 0) {
                servo_angle = 90;
            } else {
                servo_angle = 0;
            }

            // Establecer el nuevo ángulo del servo
            set_servo_angle(Servo_PIN, servo_angle);

        }
    }
    gpio_acknowledge_irq(gpio, events);  
}

int main() {
	// STDIO initialization
    stdio_init_all();

    // Initialize the GPIO input pin
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN); // Set the direction as input
    gpio_pull_up(BUTTON); // Enable pull-up

    // Initialize the PWM pin
    project_pwm_init(Servo_PIN);
    set_servo_angle(Servo_PIN, servo_angle); // Set the initial angle to 0°

    gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_LDR);
    
    while (1) {
        tight_loop_contents(); 
    }
	
    return 0;
}



