# rp2040-peripheral example.  Build:
#   export PICO_SDK_PATH=/path/to/pico-sdk
#   cmake -B build -S . && cmake --build build
# Flash build/bbp_peripheral.uf2 to a Pico in BOOTSEL mode.
cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(bbp_rp2040_peripheral C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_subdirectory(../../platforms/rp2040 bbp_rp2040)

add_executable(bbp_peripheral main.c)
target_link_libraries(bbp_peripheral bbp_rp2040 pico_stdlib hardware_adc)

pico_enable_stdio_usb(bbp_peripheral 1)    # debug console over USB CDC
pico_enable_stdio_uart(bbp_peripheral 0)   # keep UART free for the bus
pico_add_extra_outputs(bbp_peripheral)
