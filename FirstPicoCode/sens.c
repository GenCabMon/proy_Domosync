#include "sens.h"

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

void ADC_init(uint ADC_GPIO)
{
    // Configuración del ADC
    adc_init();
    adc_gpio_init(ADC_GPIO);   // GPIO 26 como entrada analógica
    adc_select_input(0); // Selecciona el canal 0 del ADC
    adc_fifo_setup(
        true,  // Habilita FIFO
        false, // No usa DMA
        1,     // Umbral de FIFO en 1
        false, // No incluir errores en FIFO
        false  // No reduce resolución a 8 bits
    );

    // Configurar el divisor del reloj para una FS de 8 kHz (48 MHz / 6000 = 8 kHz)
    adc_set_clkdiv((float)ADC_CLKDIV);

    // Configura la interrupción
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler);
    irq_set_priority(ADC_IRQ_FIFO, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_irq_set_enabled(true);

    // Iniciar el ADC
    adc_run(true);
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