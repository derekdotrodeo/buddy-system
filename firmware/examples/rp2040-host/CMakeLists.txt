# rp2040-host example.  Build:
#   export PICO_SDK_PATH=/path/to/pico-sdk
#   cmake -B build -S . && cmake --build build
# Flash build/bbp_host.uf2 to a Pico in BOOTSEL mode.
cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(bbp_rp2040_host C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_subdirectory(../../platforms/rp2040 bbp_rp2040)

add_executable(bbp_host main.c)
target_link_libraries(bbp_host bbp_rp2040 pico_stdlib)

pico_enable_stdio_usb(bbp_host 1)    # console over USB CDC
pico_enable_stdio_uart(bbp_host 0)   # keep UART free for the bus
pico_add_extra_outputs(bbp_host)     # .uf2, .elf, .map, ...
