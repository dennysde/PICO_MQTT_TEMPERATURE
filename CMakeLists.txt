# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C standards
project(pico_mqtt_tem_read C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#If Raspberry Pi Pico W is used
set(PICO_BOARD pico_w)
#If Raspberry Pi Pico is used
#set(PICO_BOARD pico)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(pico_mqtt_tem_read main.c)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(pico_mqtt_tem_read)

# Enable usb output, disable uart output for printf
pico_enable_stdio_usb(pico_mqtt_tem_read 1)
pico_enable_stdio_uart(pico_mqtt_tem_read 0)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(pico_mqtt_tem_read
        tinyusb_board
        pico_lwip_mqtt
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        hardware_adc
    )

target_include_directories(pico_mqtt_tem_read PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)