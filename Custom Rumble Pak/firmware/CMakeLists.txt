cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK (set PICO_SDK_PATH in your environment).
include(pico_sdk_import.cmake)

project(n64_rumble C CXX ASM)

pico_sdk_init()

add_executable(n64_rumble
    main.c
)

# Generate the C header from the PIO assembly.
pico_generate_pio_header(n64_rumble ${CMAKE_CURRENT_LIST_DIR}/n64_rumble.pio)

target_link_libraries(n64_rumble
    pico_stdlib
    hardware_pio
)

# Produce .uf2 / .elf / .bin etc.
pico_add_extra_outputs(n64_rumble)
