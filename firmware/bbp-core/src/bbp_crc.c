/*
 * bbp_crc.c — CRC-16/CCITT-FALSE.
 *
 * poly=0x1021, init=0xFFFF, no input/output reflection, xorout=0x0000.
 * Canonical check: CRC("123456789") == 0x29B1.
 */
#include "bbp.h"

uint16_t bbp_crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    size_t i;

    if (data == NULL) {
        return crc;
    }
    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        int bit;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}
