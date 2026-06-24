cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK (set PICO_SDK_PATH in your environment).
include(pico_sdk_import.cmake)

project(n64_rumble C CXX ASM)

pico_sdk_init()

# Bench bring-up option: USB-CDC logging of bus writes + a heartbeat.
# Do NOT enable for the finished pak (no USB host, wastes power).
#   cmake -B build -S . -DRUMBLE_DEBUG=ON
option(RUMBLE_DEBUG "Enable USB-CDC debug logging" OFF)

add_executable(n64_rumble
    main.c
)

# Generate the C header from the PIO assembly.
pico_generate_pio_header(n64_rumble ${CMAKE_CURRENT_LIST_DIR}/n64_rumble.pio)

target_link_libraries(n64_rumble
    pico_stdlib
    hardware_pio
)

if (RUMBLE_DEBUG)
    target_compile_definitions(n64_rumble PRIVATE RUMBLE_DEBUG=1)
    pico_enable_stdio_usb(n64_rumble 1)   # log over USB CDC
    pico_enable_stdio_uart(n64_rumble 0)
    message(STATUS "n64_rumble: RUMBLE_DEBUG build (USB-CDC logging enabled)")
endif()

# Motor PWM: intensity cap + soft-start (instead of plain on/off).
#   cmake -B build -S . -DRUMBLE_PWM=ON -DRUMBLE_DUTY=80 -DRUMBLE_SOFTSTART_MS=3
option(RUMBLE_PWM "Drive the motor with PWM (intensity + soft-start)" OFF)
set(RUMBLE_DUTY "100" CACHE STRING "PWM duty percent when rumbling (1-100)")
set(RUMBLE_SOFTSTART_MS "0" CACHE STRING "Soft-start ramp time in ms (0 = off)")
if (RUMBLE_PWM)
    target_compile_definitions(n64_rumble PRIVATE
        RUMBLE_PWM=1
        RUMBLE_DUTY=${RUMBLE_DUTY}
        RUMBLE_SOFTSTART_MS=${RUMBLE_SOFTSTART_MS})
    target_link_libraries(n64_rumble hardware_pwm)
    message(STATUS "n64_rumble: RUMBLE_PWM build (duty=${RUMBLE_DUTY}%, soft-start=${RUMBLE_SOFTSTART_MS}ms)")
endif()

# Produce .uf2 / .elf / .bin etc.
pico_add_extra_outputs(n64_rumble)
