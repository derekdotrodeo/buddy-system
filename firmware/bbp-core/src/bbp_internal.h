/*
 * bbp_internal.h — private helpers shared across core translation units.
 * Not part of the public API.
 */
#ifndef BBP_INTERNAL_H
#define BBP_INTERNAL_H

#include "bbp.h"

/* Valid as a SRC: a real slot (1..6) or the host. Never broadcast. */
static inline bool bbp_valid_src(uint8_t a)
{
    return (a >= BBP_SLOT_MIN && a <= BBP_SLOT_MAX) || a == BBP_HOST_ADDR;
}

/* Valid as a DEST: broadcast, a real slot, or the host. */
static inline bool bbp_valid_dest(uint8_t a)
{
    return a == BBP_BROADCAST ||
           (a >= BBP_SLOT_MIN && a <= BBP_SLOT_MAX) ||
           a == BBP_HOST_ADDR;
}

#endif /* BBP_INTERNAL_H */
