/*
 * bbp_rp2040_slot.h — pure SLOT-voltage -> slot-number mapping.
 *
 * Separated from the SDK-dependent platform so it can be unit-tested on the
 * host. No Pico SDK dependency.
 */
#ifndef BBP_RP2040_SLOT_H
#define BBP_RP2040_SLOT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Map a SLOT ADC reading to a slot number 1..6 by thresholding at the midpoints
   between the nominal levels in BS-SPEC-100 §6.3. `full` is the ADC full-scale
   count (e.g. 4095 for the RP2040's 12-bit ADC). Returns 0 only if full == 0. */
uint8_t bbp_rp2040_slot_from_raw(uint16_t raw, uint16_t full);

#ifdef __cplusplus
}
#endif

#endif /* BBP_RP2040_SLOT_H */
