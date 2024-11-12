cmake_minimum_required(VERSION 3.13)

# Define the path to the Pico SDK
set(PICO_SDK_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/pico-sdk")

# Include the Pico SDK import file
include(pico_sdk_import.cmake)

# Define the project name and language
project(sensTemp C CXX)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add the executable target
add_executable(control_PI_fan
    control_PI_fan.c
)

string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

# Link the Pico standard library
target_link_libraries(control_PI_fan m pico_stdlib hardware_adc hardware_timer hardware_gpio hardware_pwm hardware_irq hardware_sync pico_time)

# Configurar la salida estándar (UART y USB)
pico_enable_stdio_usb(control_PI_fan 1)
pico_enable_stdio_uart(control_PI_fan 0)

# Generate additional output files (map, bin, hex, uf2)
pico_add_extra_outputs(control_PI_fan)