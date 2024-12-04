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
// Include your own header files here

/**
 * @brief Main program.
 *
 * This function initializes the MCU and does an infinite cycle.
 */

#define LDR_PIN 16
#define LED_OUT_PIN 15 

// ISR para manejar flancos de subida y bajada
void gpio_callback_LDR(uint gpio, uint32_t events) {
    printf("GPIO %d, event %d\n", gpio, events);
    if (gpio == LDR_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {  // Flanco de subida (LOW -> HIGH)
            gpio_put(LED_OUT_PIN, 1);
        } else if (events & GPIO_IRQ_EDGE_FALL) {  // Flanco de bajada (HIGH -> LOW)
            gpio_put(LED_OUT_PIN, 0);
        }
    }
    gpio_acknowledge_irq(gpio, events);  
}

int main() {
	// STDIO initialization
    stdio_init_all();
	
	// Write your initialization code here
    printf("Digital, measuring GPIO 16\n");
    printf("LED, output GPIO 15\n");

    // Initialize the GPIO input pin
    gpio_init(LDR_PIN);
    gpio_set_dir(LDR_PIN, GPIO_IN); // Set the direction as input
    gpio_pull_up(LDR_PIN); // Enable pull-up

    // Initialize the GPIO out pin
    gpio_init(LED_OUT_PIN);
    gpio_set_dir(LED_OUT_PIN, GPIO_OUT); // Set the direction as output

    gpio_set_irq_enabled_with_callback(LDR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback_LDR);


	// Infinite loop. This function shouldn't finish or return
    while (1) {
        tight_loop_contents(); 
    }
	
    return 0;
}
