cmake_minimum_required(VERSION 3.13)

# Always include it
include(pico_sdk_import.cmake)

# Project's name (Replace my_DE2_Project with your own project's name)
project(Reading_LDR_DIG C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# SDK Initialization - Mandatory
pico_sdk_init()

# C/C++ project files
add_executable(Reading_LDR_DIG
    main.c
)

# pico_stdlib library. You can add more if they are needed
target_link_libraries(Reading_LDR_DIG pico_stdlib gpio)

# Enable usb output, disable uart output
pico_enable_stdio_usb(Reading_LDR_DIG 1)
pico_enable_stdio_uart(Reading_LDR_DIG 0)

# Need to generate UF2 file for upload to RP2040
pico_add_extra_outputs(Reading_LDR_DIG)