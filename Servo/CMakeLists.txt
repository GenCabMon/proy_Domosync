cmake_minimum_required(VERSION 3.13)

# Always include it
include(pico_sdk_import.cmake)

# Project's name (Replace my_DE2_Project with your own project's name)
project(PWM_servo C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# SDK Initialization - Mandatory
pico_sdk_init()

# C/C++ project files
add_executable(PWM_servo
    main.c
)

# pico_stdlib library. You can add more if they are needed
target_link_libraries(PWM_servo pico_stdlib hardware_gpio hardware_pwm)

# Enable usb output, disable uart output
pico_enable_stdio_usb(PWM_servo 1)
pico_enable_stdio_uart(PWM_servo 0)

# Need to generate UF2 file for upload to RP2040
pico_add_extra_outputs(PWM_servo)