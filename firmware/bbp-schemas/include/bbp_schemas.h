/*
 * bbp_schemas.h — common application payload schemas (BS-SPEC-300).
 *
 * Header-only codecs for the baseline catalog of interoperable schemas. These
 * are recommended, not required: a module may use them, extend them, or define
 * its own (BS-SPEC-200 §11). When a module uses one of these schema names it
 * shall use the layout defined here.
 *
 * Standalone — depends only on the C standard library. Multi-byte integers are
 * big-endian, matching BBP framing (BS-SPEC-200 §5.1). Each codec is an event
 * payload: one event per packet, drained by the host's polls.
 *
 * Dispatch on the schema NAME first (two payloads can share a length), then
 * decode: e.g. if (bbp_schema_eq(pkt, BBP_SCHEMA_POT)) bbp_pot_decode(...).
 */
#ifndef BBP_SCHEMAS_H
#define BBP_SCHEMAS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* schema names: controls.* for physical controls, midi.* for MIDI */
#define BBP_SCHEMA_POT      "controls.pot"
#define BBP_SCHEMA_BUTTON   "controls.button"
#define BBP_SCHEMA_ENCODER  "controls.encoder"
#define BBP_SCHEMA_MIDI     "midi.message"

/* ---- controls.pot : [index:u8][value:u16 BE] — absolute analog position ---- */
#define BBP_POT_LEN 3u
static inline uint8_t bbp_pot_encode(uint8_t *buf, uint8_t index, uint16_t value)
{
    buf[0] = index;
    buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)(value & 0xFFu);
    return BBP_POT_LEN;
}
static inline bool bbp_pot_decode(const uint8_t *p, uint8_t len,
                                  uint8_t *index, uint16_t *value)
{
    if (p == NULL || len != BBP_POT_LEN) {
        return false;
    }
    if (index != NULL) { *index = p[0]; }
    if (value != NULL) { *value = (uint16_t)(((uint16_t)p[1] << 8) | p[2]); }
    return true;
}

/* ---- controls.button : [index:u8][state:u8] (1 = pressed, 0 = released) ---- */
#define BBP_BUTTON_LEN 2u
static inline uint8_t bbp_button_encode(uint8_t *buf, uint8_t index, bool pressed)
{
    buf[0] = index;
    buf[1] = pressed ? 1u : 0u;
    return BBP_BUTTON_LEN;
}
static inline bool bbp_button_decode(const uint8_t *p, uint8_t len,
                                     uint8_t *index, bool *pressed)
{
    if (p == NULL || len != BBP_BUTTON_LEN) {
        return false;
    }
    if (index != NULL)   { *index = p[0]; }
    if (pressed != NULL) { *pressed = (p[1] != 0u); }
    return true;
}

/* ---- controls.encoder : [index:u8][delta:i8] — signed detents since last ---- */
#define BBP_ENCODER_LEN 2u
static inline uint8_t bbp_encoder_encode(uint8_t *buf, uint8_t index, int8_t delta)
{
    buf[0] = index;
    buf[1] = (uint8_t)delta;
    return BBP_ENCODER_LEN;
}
static inline bool bbp_encoder_decode(const uint8_t *p, uint8_t len,
                                      uint8_t *index, int8_t *delta)
{
    if (p == NULL || len != BBP_ENCODER_LEN) {
        return false;
    }
    if (index != NULL) { *index = p[0]; }
    if (delta != NULL) { *delta = (int8_t)p[1]; }
    return true;
}

/* ---- midi.message : raw MIDI message bytes (status + data), one per packet.
   The payload is the message verbatim, so no codec is needed beyond the name.
   Running status and SysEx are out of scope for v0.1 (see BS-SPEC-300 §6). ---- */

#ifdef __cplusplus
}
#endif

#endif /* BBP_SCHEMAS_H */
