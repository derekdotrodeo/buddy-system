/*
 * bbp_info.c — bbp.info payload encode/decode (BS-SPEC-200 §9.3).
 *
 * Wire layout:
 *   [0] slot_count  [1] version_major  [2] version_minor
 *   [3] vendor_len  [4..] vendor       [.] product_len  [..] product
 */
#include "bbp_policy.h"
#include <string.h>

static uint8_t name_len(const char *s)
{
    size_t n = (s != NULL) ? strlen(s) : 0u;
    return (n > BBP_NAME_MAX) ? (uint8_t)BBP_NAME_MAX : (uint8_t)n;
}

uint8_t bbp_info_encode(uint8_t *buf, const bbp_identity_t *id)
{
    uint8_t vlen, plen, i = 0;

    if (buf == NULL || id == NULL) {
        return 0;
    }
    vlen = name_len(id->vendor);
    plen = name_len(id->product);

    buf[i++] = id->slot_count;
    buf[i++] = id->version_major;
    buf[i++] = id->version_minor;
    buf[i++] = vlen;
    if (vlen > 0u) { memcpy(&buf[i], id->vendor, vlen); i = (uint8_t)(i + vlen); }
    buf[i++] = plen;
    if (plen > 0u) { memcpy(&buf[i], id->product, plen); i = (uint8_t)(i + plen); }
    return i;
}

bool bbp_info_decode(const uint8_t *p, uint8_t len, bbp_info_t *out)
{
    uint8_t i = 0, vlen, plen;

    if (p == NULL || out == NULL || len < 5u) {   /* min: 3 + vlen(0) + plen(0) */
        return false;
    }
    memset(out, 0, sizeof *out);
    out->slot_count    = p[i++];
    out->version_major = p[i++];
    out->version_minor = p[i++];

    vlen = p[i++];
    if (vlen > BBP_NAME_MAX || (uint16_t)i + vlen > len) {
        return false;
    }
    if (vlen > 0u) { memcpy(out->vendor, &p[i], vlen); i = (uint8_t)(i + vlen); }
    out->vendor_len = vlen;

    if (i >= len) {
        return false;                              /* missing product_len */
    }
    plen = p[i++];
    if (plen > BBP_NAME_MAX || (uint16_t)i + plen > len) {
        return false;
    }
    if (plen > 0u) { memcpy(out->product, &p[i], plen); }
    out->product_len = plen;
    return true;
}
