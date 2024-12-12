#include "digi_elements.h"

// Funciones de inicialización de sensores
void set_up_LDR()
{
    // Initialize the GPIO input pin
    gpio_init(LDR_PIN);
    gpio_set_dir(LDR_PIN, GPIO_IN); // Set the direction as input
    gpio_pull_down(LDR_PIN);        // Enable pull-up

    // Initialize the GPIO out pin
    gpio_init(LED_OUT_PIN);
    gpio_set_dir(LED_OUT_PIN, GPIO_OUT); // Set the direction as output
}

void set_up_IR()
{
    // Initialize the GPIO input pin
    gpio_init(IR_PIN);
    gpio_set_dir(IR_PIN, GPIO_IN); // Set the direction as input
    gpio_pull_down(IR_PIN);        // Enable pull-up

    // Initialize the GPIO out pin
    gpio_init(LED_OUT_PIN_IR);
    gpio_set_dir(LED_OUT_PIN_IR, GPIO_OUT); // Set the direction as output
}

void LandB_init()
{
    printf("Begin code\n");

    // Inicializamos el GPIO del LED como salida
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(LED_PIN_2);
    gpio_set_dir(LED_PIN_2, GPIO_OUT);

    // Initialize the GPIO input pin
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN); // Set the direction as input
    gpio_pull_up(BUTTON);          // Enable pull-up
}