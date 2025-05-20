# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
set(FREERTOS_KERNEL_PATH "C:/Users/malub/FreeRTOS-Kernel")
include(C:/Users/malub/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Tarefa5 C CXX ASM)
pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add executable. Default name is the project name, version 0.1
add_executable(Tarefa5
    Tarefa5.c
    lib/ssd1306.c 
    )

# Add the standard include files to the build
target_include_directories(Tarefa5 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Gerar cabeçalho PIO
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(Tarefa5 ${CMAKE_CURRENT_LIST_DIR}/Tarefa5.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

pico_set_program_name(Tarefa5 "Tarefa5")
pico_set_program_version(Tarefa5 "0.1")

# Add any user requested libraries
target_link_libraries(Tarefa5
        pico_stdlib 
        hardware_pio
        hardware_gpio
        hardware_i2c
        hardware_pwm
        hardware_adc
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        )

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Tarefa5 1)
pico_enable_stdio_usb(Tarefa5 1)

pico_add_extra_outputs(Tarefa5)

