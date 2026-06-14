# BBP RP2040 platform library.
#
# Add this to a Pico SDK project with:
#   add_subdirectory(<path>/firmware/platforms/rp2040 bbp_rp2040)
#   target_link_libraries(<your_exe> bbp_rp2040 pico_stdlib)
#
# It bundles bbp-core + bbp-policy + the RP2040 platform into one static library.

set(BBP_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

add_library(bbp_rp2040 STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/bbp_rp2040.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bbp_rp2040_slot.c
    ${BBP_ROOT}/bbp-core/src/bbp_crc.c
    ${BBP_ROOT}/bbp-core/src/bbp_rx.c
    ${BBP_ROOT}/bbp-core/src/bbp_tx.c
    ${BBP_ROOT}/bbp-core/src/bbp_util.c
    ${BBP_ROOT}/bbp-policy/src/bbp_host.c
    ${BBP_ROOT}/bbp-policy/src/bbp_mod.c
    ${BBP_ROOT}/bbp-policy/src/bbp_info.c
)

target_include_directories(bbp_rp2040 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${BBP_ROOT}/bbp-core/include
    ${BBP_ROOT}/bbp-policy/include
    ${BBP_ROOT}/bbp-schemas/include
)

target_link_libraries(bbp_rp2040 PUBLIC
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_adc
    hardware_irq
)
