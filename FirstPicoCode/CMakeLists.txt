cmake_minimum_required(VERSION 3.13)

# Define the path to the Pico SDK
set(PICO_SDK_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/pico-sdk")

# Include the Pico SDK import file
include(pico_sdk_import.cmake)

# Define the project name and language
project(keypad C CXX)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add the executable target
add_executable(measure
        measure.c
        measurelibs.c
        base_de_datos.c
        digiElements.c
        configpwm.c
)

string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

# Link the Pico standard library
target_link_libraries(measure
        pico_stdlib    # Librería estándar para Raspberry Pi Pico
        hardware_adc   # Controlador del ADC
        hardware_gpio
        m
        hardware_irq   # Biblioteca específica para manejo de interrupciones
        hardware_sync  # Biblioteca para funciones de sincronización
        hardware_pwm
)
# Configurar la salida estándar (UART y USB)
pico_enable_stdio_usb(measure 1)
pico_enable_stdio_uart(measure 0)

# Generate additional output files (map, bin, hex, uf2)
pico_add_extra_outputs(measure)