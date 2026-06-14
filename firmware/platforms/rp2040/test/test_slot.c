/*
 * test_slot.c — host unit test for the SLOT mapping (the SDK-free part of the
 * RP2040 platform). Run with `make test` from firmware/platforms/rp2040.
 */
#include "bbp_rp2040_slot.h"
#include <stdio.h>

static int failures;

#define CHECK(cond) do {                                              \
    if (!(cond)) {                                                    \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        failures++;                                                   \
    }                                                                 \
} while (0)

/* raw count for a fraction of full-scale */
static uint16_t at(double frac, uint16_t full)
{
    return (uint16_t)(frac * (double)full);
}

int main(void)
{
    const uint16_t full = 4095u;

    printf("test_slot_mapping\n");

    /* each nominal level maps to its own slot (BS-SPEC-100 §6.3) */
    CHECK(bbp_rp2040_slot_from_raw(at(0.091, full), full) == 1);
    CHECK(bbp_rp2040_slot_from_raw(at(0.180, full), full) == 2);
    CHECK(bbp_rp2040_slot_from_raw(at(0.320, full), full) == 3);
    CHECK(bbp_rp2040_slot_from_raw(at(0.500, full), full) == 4);
    CHECK(bbp_rp2040_slot_from_raw(at(0.688, full), full) == 5);
    CHECK(bbp_rp2040_slot_from_raw(at(0.825, full), full) == 6);

    /* rails clamp to the end slots */
    CHECK(bbp_rp2040_slot_from_raw(0, full) == 1);
    CHECK(bbp_rp2040_slot_from_raw(full, full) == 6);
    CHECK(bbp_rp2040_slot_from_raw(full + 100u, full) == 6);   /* over-range clamps */

    /* either side of the slot1/slot2 midpoint (~0.1355) */
    CHECK(bbp_rp2040_slot_from_raw(at(0.130, full), full) == 1);
    CHECK(bbp_rp2040_slot_from_raw(at(0.145, full), full) == 2);
    /* either side of the slot2/slot3 midpoint (~0.250) */
    CHECK(bbp_rp2040_slot_from_raw(at(0.245, full), full) == 2);
    CHECK(bbp_rp2040_slot_from_raw(at(0.260, full), full) == 3);

    /* degenerate full-scale */
    CHECK(bbp_rp2040_slot_from_raw(123, 0) == 0);

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
}
