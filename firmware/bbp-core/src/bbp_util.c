/*
 * bbp_util.c — lifecycle, schema compare, stats, big-endian accessors.
 */
#include "bbp.h"
#include <string.h>

bbp_err_t bbp_init(bbp_t *b, const bbp_platform_t *plat, uint8_t self_addr)
{
    if (b == NULL || plat == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (plat->tx == NULL) {
        return BBP_ERR_PLATFORM;
    }
    memset(b, 0, sizeof(*b));   /* rx_state = 0 = HUNT_SOF */
    b->plat = plat;
    b->self_addr = self_addr;
    return BBP_OK;
}

bbp_err_t bbp_set_self_addr(bbp_t *b, uint8_t self_addr)
{
    if (b == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    b->self_addr = self_addr;
    return BBP_OK;
}

bool bbp_schema_eq(const bbp_packet_t *p, const char *s)
{
    size_t n;
    if (p == NULL || s == NULL) {
        return false;
    }
    n = strlen(s);
    if (n != (size_t)p->schema_len) {
        return false;
    }
    return memcmp(p->schema, s, n) == 0;
}

void bbp_get_stats(const bbp_t *b, bbp_stats_t *out)
{
    if (b == NULL || out == NULL) {
        return;
    }
    *out = b->stats;
}

void bbp_put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

void bbp_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

uint16_t bbp_get_u16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

uint32_t bbp_get_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}
