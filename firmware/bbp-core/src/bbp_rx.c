/*
 * bbp_rx.c — receive-path frame assembler.
 *
 * Fed one byte at a time. No byte-stuffing: framing trusts the length fields
 * plus CRC, and rescans for SOF after any fault. A single working buffer holds
 * the CRC-covered body (SRC..PAYLOAD); the CRC bytes are compared, not stored.
 *
 * Resync detail: when a field guard fails, the offending byte is REPROCESSED in
 * HUNT_SOF, so a stray 0x7E that tripped a guard is recognised as a new SOF
 * rather than discarded.
 */
#include "bbp.h"
#include "bbp_internal.h"
#include <string.h>

enum {
    BBP_RX_HUNT_SOF = 0,   /* must be 0: bbp_init memsets the instance */
    BBP_RX_SRC,
    BBP_RX_DEST,
    BBP_RX_SCHEMA_LEN,
    BBP_RX_SCHEMA,
    BBP_RX_PAYLOAD_LEN,
    BBP_RX_PAYLOAD,
    BBP_RX_CRC_HI,
    BBP_RX_CRC_LO
};

static void start_frame(bbp_t *b)
{
    if (b->rx_ready) {
        /* dropping an unread completed packet to accept a newer frame */
        b->stats.rx_overrun++;
        b->rx_ready = false;
    }
    b->rx_pos = 0;
    b->rx_state = BBP_RX_SRC;
}

bbp_err_t bbp_receive_byte(bbp_t *b, uint8_t byte)
{
    bool reprocess;

    if (b == NULL) {
        return BBP_ERR_NULL_ARG;
    }

    do {
        reprocess = false;

        switch (b->rx_state) {
        case BBP_RX_HUNT_SOF:
            if (byte == BBP_SOF) {
                start_frame(b);
            }
            break;

        case BBP_RX_SRC:
            if (!bbp_valid_src(byte)) {
                b->stats.rx_frame_err++;
                b->rx_state = BBP_RX_HUNT_SOF;
                reprocess = true;
                break;
            }
            b->rx_buf[0] = byte;
            b->rx_pos = 1;
            b->rx_state = BBP_RX_DEST;
            break;

        case BBP_RX_DEST:
            if (!bbp_valid_dest(byte)) {
                b->stats.rx_frame_err++;
                b->rx_state = BBP_RX_HUNT_SOF;
                reprocess = true;
                break;
            }
            b->rx_buf[1] = byte;
            b->rx_pos = 2;
            b->rx_state = BBP_RX_SCHEMA_LEN;
            break;

        case BBP_RX_SCHEMA_LEN:
            if (byte > BBP_MAX_SCHEMA) {
                b->stats.rx_frame_err++;
                b->rx_state = BBP_RX_HUNT_SOF;
                reprocess = true;
                break;
            }
            b->rx_schema_len = byte;
            b->rx_buf[2] = byte;
            b->rx_pos = 3;
            if (byte == 0) {
                b->rx_state = BBP_RX_PAYLOAD_LEN;
            } else {
                b->rx_need = byte;
                b->rx_state = BBP_RX_SCHEMA;
            }
            break;

        case BBP_RX_SCHEMA:
            b->rx_buf[b->rx_pos++] = byte;
            if (--b->rx_need == 0) {
                b->rx_state = BBP_RX_PAYLOAD_LEN;
            }
            break;

        case BBP_RX_PAYLOAD_LEN:
            b->rx_payload_len = byte;
            b->rx_buf[b->rx_pos++] = byte;
            b->rx_payload_off = b->rx_pos;
            if (byte == 0) {
                b->rx_state = BBP_RX_CRC_HI;
            } else {
                b->rx_need = byte;
                b->rx_state = BBP_RX_PAYLOAD;
            }
            break;

        case BBP_RX_PAYLOAD:
            b->rx_buf[b->rx_pos++] = byte;
            if (--b->rx_need == 0) {
                b->rx_state = BBP_RX_CRC_HI;
            }
            break;

        case BBP_RX_CRC_HI:
            b->rx_crc_hi = byte;
            b->rx_state = BBP_RX_CRC_LO;
            break;

        case BBP_RX_CRC_LO: {
            uint16_t got = (uint16_t)((uint16_t)b->rx_crc_hi << 8 | byte);
            uint16_t want = bbp_crc16_ccitt(b->rx_buf, b->rx_pos);
            if (got == want) {
                b->stats.rx_good++;
                b->rx_ready = true;   /* awaits bbp_poll */
            } else {
                b->stats.rx_crc_err++;
            }
            b->rx_state = BBP_RX_HUNT_SOF;
            break;
        }

        default:
            b->rx_state = BBP_RX_HUNT_SOF;
            break;
        }
    } while (reprocess);

    return BBP_OK;
}

bbp_err_t bbp_poll(bbp_t *b, bbp_packet_t *out)
{
    if (b == NULL || out == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (!b->rx_ready) {
        return BBP_ERR_NO_PACKET;
    }

    out->src         = b->rx_buf[0];
    out->dest        = b->rx_buf[1];
    out->schema_len  = b->rx_schema_len;
    out->payload_len = b->rx_payload_len;
    if (b->rx_schema_len > 0) {
        memcpy(out->schema, &b->rx_buf[3], b->rx_schema_len);
    }
    if (b->rx_payload_len > 0) {
        memcpy(out->payload, &b->rx_buf[b->rx_payload_off], b->rx_payload_len);
    }

    b->rx_ready = false;   /* release the working buffer */
    return BBP_OK;
}
