/*
 * bbp_tx.c — transmit path: encode a frame, then transmit it.
 *
 * Encode is pure (builds bytes into the TX buffer, no hardware). Transmit owns
 * the half-duplex turnaround: assert DE, block-send, deassert DE.
 */
#include "bbp.h"
#include "bbp_internal.h"
#include <string.h>

bbp_err_t bbp_encode(bbp_t *b, uint8_t dest,
                     const char *schema, uint8_t schema_len,
                     const uint8_t *payload, uint8_t payload_len)
{
    uint16_t pos;
    uint16_t crc;

    if (b == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (b->plat == NULL) {
        return BBP_ERR_NOT_INIT;
    }
    if ((schema_len > 0 && schema == NULL) ||
        (payload_len > 0 && payload == NULL)) {
        return BBP_ERR_NULL_ARG;
    }
    if (schema_len > BBP_MAX_SCHEMA) {
        return BBP_ERR_SCHEMA_TOO_LONG;
    }
    if (payload_len > BBP_MAX_PAYLOAD) {
        return BBP_ERR_PAYLOAD_TOO_LONG;
    }
    if (!bbp_valid_src(b->self_addr) || !bbp_valid_dest(dest)) {
        return BBP_ERR_INVALID_ADDR;
    }

    pos = 0;
    b->tx_buf[pos++] = BBP_SOF;
    b->tx_buf[pos++] = b->self_addr;     /* SRC */
    b->tx_buf[pos++] = dest;
    b->tx_buf[pos++] = schema_len;
    if (schema_len > 0) {
        memcpy(&b->tx_buf[pos], schema, schema_len);
        pos = (uint16_t)(pos + schema_len);
    }
    b->tx_buf[pos++] = payload_len;
    if (payload_len > 0) {
        memcpy(&b->tx_buf[pos], payload, payload_len);
        pos = (uint16_t)(pos + payload_len);
    }

    /* CRC over SRC..PAYLOAD = tx_buf[1 .. pos-1] */
    crc = bbp_crc16_ccitt(&b->tx_buf[1], (size_t)(pos - 1));
    b->tx_buf[pos++] = (uint8_t)(crc >> 8);   /* CRC_HI (big-endian) */
    b->tx_buf[pos++] = (uint8_t)(crc & 0xFF); /* CRC_LO */

    b->tx_len = pos;
    return BBP_OK;
}

bbp_err_t bbp_transmit(bbp_t *b)
{
    if (b == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (b->plat == NULL || b->plat->tx == NULL) {
        return BBP_ERR_NOT_INIT;
    }
    if (b->tx_len == 0) {
        return BBP_ERR_TX_BUF_EMPTY;
    }

    if (b->plat->set_driver_enable != NULL) {
        b->plat->set_driver_enable(b->plat->ctx, true);
    }
    b->plat->tx(b->plat->ctx, b->tx_buf, b->tx_len);   /* blocks until wire drained */
    if (b->plat->set_driver_enable != NULL) {
        b->plat->set_driver_enable(b->plat->ctx, false);
    }

    b->stats.tx_count++;
    return BBP_OK;
}
