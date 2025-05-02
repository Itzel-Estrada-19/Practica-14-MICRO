cmake_minimum_required(VERSION 3.13)

# Incluir el Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

# Inicializar el SDK
pico_sdk_init()

project(Practica_14 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(Practica_14
    semaforo.c
)

# Agregar librerías necesarias
target_link_libraries(Practica_14
    pico_stdlib
    pico_multicore   # Soporte para multicore
)

# Generar binarios, uf2, etc.
pico_add_extra_outputs(Practica_14)