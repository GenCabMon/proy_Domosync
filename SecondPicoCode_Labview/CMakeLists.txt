cmake_minimum_required(VERSION 3.13)

# Define the path to the Pico SDK
set(PICO_SDK_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/pico-sdk")

# Include the Pico SDK import file
include(pico_sdk_import.cmake)

# Define the project name and language
project(AccessSys C CXX)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(access
	main_pico.c
	LCD_i2c.c
	base_de_datos.c
	access_system.c
	Functions.c

)


string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(access pico_stdlib hardware_timer hardware_adc hardware_gpio hardware_pwm hardware_irq hardware_sync hardware_i2c hardware_spi)

pico_enable_stdio_uart(access 0)
pico_enable_stdio_usb(access 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(access)