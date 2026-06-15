/*
 * bbp_rp2040_slot.c — SLOT-voltage -> slot-number mapping (pure, host-testable).
 */
#include "bbp_rp2040_slot.h"

uint8_t bbp_rp2040_slot_from_raw(uint16_t raw, uint16_t full)
{
    /* BS-SPEC-100 §6.3 nominal SLOT levels, as a fraction of 3V3:
         slot 1=0.091  2=0.180  3=0.320  4=0.500  5=0.688  6=0.825
       Threshold at the midpoints between adjacent levels (per-mille):  */
    static const uint16_t mid[5] = { 136u, 250u, 410u, 594u, 757u };
    uint32_t permille;
    uint8_t  i;

    if (full == 0u) {
        return 0u;
    }
    if (raw > full) {
        raw = full;
    }
    permille = (uint32_t)raw * 1000u / full;
    for (i = 0; i < 5u; i++) {
        if (permille < mid[i]) {
            return (uint8_t)(i + 1u);
        }
    }
    return 6u;
}
