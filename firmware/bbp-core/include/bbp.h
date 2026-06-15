/*
 * bbp.h — Buddy Bus Protocol (BBP) Core, public API.
 *
 * Platform-independent C99. No dynamic memory. No hardware access.
 * The core assembles/validates received frames (bytes fed in one at a time)
 * and encodes/transmits frames (via an injected platform interface).
 *
 * See firmware design document for the full architecture. Wire format:
 *
 *   SOF | SRC | DEST | SCHEMA_LEN | SCHEMA | PAYLOAD_LEN | PAYLOAD | CRC_HI CRC_LO
 *
 * SOF        = 0x7E (constant, not CRC-covered)
 * CRC        = CRC-16/CCITT-FALSE over SRC..PAYLOAD inclusive, big-endian on wire
 * addresses  = slots 1..6, 0 = broadcast (DEST only), 0x07 = host
 */
#ifndef BBP_H
#define BBP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- protocol constants ---- */
#define BBP_VERSION        1u      /* BBP protocol version (negotiated in bbp.info) */
#define BBP_SOF            0x7Eu   /* start-of-frame delimiter */
#define BBP_BROADCAST      0x00u   /* DEST = broadcast (no response permitted) */
#define BBP_HOST_ADDR      0x07u   /* the Buddy Base / host address */
#define BBP_SLOT_MIN       1u
#define BBP_SLOT_MAX       6u

#define BBP_MAX_SCHEMA     31u     /* max SCHEMA bytes */
#define BBP_MAX_PAYLOAD    255u    /* max PAYLOAD bytes */
/* SRC+DEST+SCHEMA_LEN + SCHEMA + PAYLOAD_LEN + PAYLOAD (the CRC-covered region) */
#define BBP_BODY_MAX       (3u + BBP_MAX_SCHEMA + 1u + BBP_MAX_PAYLOAD)   /* 290 */
#define BBP_MAX_FRAME      (1u + BBP_BODY_MAX + 2u)                       /* 293 */
#define BBP_MIN_FRAME      7u      /* SOF+SRC+DEST+0+0+CRC */

/* ---- return codes (no exceptions/asserts for runtime errors) ---- */
typedef enum {
    BBP_OK = 0,
    BBP_ERR_NULL_ARG,
    BBP_ERR_NOT_INIT,
    BBP_ERR_SCHEMA_TOO_LONG,       /* schema > BBP_MAX_SCHEMA */
    BBP_ERR_PAYLOAD_TOO_LONG,      /* payload > BBP_MAX_PAYLOAD */
    BBP_ERR_NO_PACKET,             /* bbp_poll: nothing ready */
    BBP_ERR_BAD_FRAME,             /* illegal field / guard trip while parsing */
    BBP_ERR_TX_BUF_EMPTY,          /* bbp_transmit with nothing encoded */
    BBP_ERR_INVALID_ADDR,          /* SRC/DEST out of range */
    BBP_ERR_PLATFORM,              /* missing required platform callback */
    BBP_ERR_QUEUE_FULL,            /* outgoing queue full (policy layer) */
} bbp_err_t;

/* ---- wire-level error classes (payload byte 0 of a bbp.error packet) ---- */
typedef enum {
    BBP_ERRC_RESERVED       = 0x00,
    BBP_ERRC_UNKNOWN_SCHEMA = 0x01, /* context = echoed schema string */
    BBP_ERRC_BAD_PAYLOAD    = 0x02,
    BBP_ERRC_BUSY           = 0x03,
    BBP_ERRC_UNSUPPORTED    = 0x04, /* version / feature not supported */
    BBP_ERRC_BAD_REQUEST    = 0x05, /* malformed request */
    BBP_ERRC_INTERNAL       = 0x06,
} bbp_errc_t;

/* ---- received packet: caller-owned storage, copied out by bbp_poll ---- */
typedef struct {
    uint8_t  src;
    uint8_t  dest;
    uint8_t  schema_len;                  /* 0..BBP_MAX_SCHEMA */
    char     schema[BBP_MAX_SCHEMA];      /* NOT NUL-terminated; use schema_len */
    uint8_t  payload_len;                 /* 0..BBP_MAX_PAYLOAD */
    uint8_t  payload[BBP_MAX_PAYLOAD];
} bbp_packet_t;

/* ---- diagnostics ---- */
typedef struct {
    uint32_t rx_good;       /* CRC-valid frames delivered */
    uint32_t rx_crc_err;    /* frames dropped on CRC mismatch */
    uint32_t rx_frame_err;  /* frames dropped on field/guard violation */
    uint32_t rx_overrun;    /* unread packets dropped to accept a newer frame */
    uint32_t tx_count;      /* frames transmitted */
} bbp_stats_t;

/*
 * Platform interface (injected at init). The core never touches hardware.
 *
 * Concurrency contract: bbp_receive_byte() and bbp_poll() are NOT internally
 * synchronized. If bytes are fed from an ISR, mask that ISR around bbp_poll(),
 * or (preferred) drain the UART into a ring buffer and feed bytes from the
 * main loop. The desktop platform is single-threaded and needs neither.
 */
typedef struct bbp_platform {
    /* Transmit len bytes. MUST block until the last byte has physically left
       the wire (UART transmission-complete, not just register-empty), so the
       core can safely drop the driver enable afterwards. */
    void     (*tx)(void *ctx, const uint8_t *buf, size_t len);
    /* Half-duplex driver control: assert/deassert DE (and ~RE) together.
       May be NULL on full-duplex / loopback platforms. */
    void     (*set_driver_enable)(void *ctx, bool on);
    /* Monotonic milliseconds. Used by host/module policy layers, not core. */
    uint32_t (*millis)(void *ctx);
    /* Read this card's slot (1..6) from the analog SLOT pin, or 0 if uncertain.
       May be NULL on the host. */
    uint8_t  (*read_slot)(void *ctx);
    /* Non-blocking receive: return the next inbound byte (0..255), or a negative
       value if none is available. Used by the host/module policy task loops to
       interleave receive with parse; the core itself is push-fed via
       bbp_receive_byte and never calls this. May be NULL on push-only platforms. */
    int      (*rx)(void *ctx);
    void     *ctx;
} bbp_platform_t;

/* ---- core instance (caller-allocated; fields are private) ---- */
typedef struct bbp {
    const bbp_platform_t *plat;
    uint8_t  self_addr;

    /* RX assembly (single working buffer; SRC..PAYLOAD only, CRC compared) */
    uint8_t  rx_state;
    uint8_t  rx_buf[BBP_BODY_MAX];
    uint16_t rx_pos;           /* write index into rx_buf */
    uint16_t rx_need;          /* remaining bytes for the current variable field */
    uint8_t  rx_schema_len;
    uint8_t  rx_payload_len;
    uint16_t rx_payload_off;   /* offset of payload within rx_buf */
    uint8_t  rx_crc_hi;        /* first received CRC byte */
    bool     rx_ready;         /* a validated frame awaits bbp_poll */

    /* TX */
    uint8_t  tx_buf[BBP_MAX_FRAME];
    uint16_t tx_len;

    bbp_stats_t stats;
} bbp_t;

/* ---- lifecycle ---- */
bbp_err_t bbp_init(bbp_t *b, const bbp_platform_t *plat, uint8_t self_addr);
bbp_err_t bbp_set_self_addr(bbp_t *b, uint8_t self_addr);

/* ---- RX ---- */
/* Feed one received byte into the assembler. ISR-safe, O(1), no platform calls. */
bbp_err_t bbp_receive_byte(bbp_t *b, uint8_t byte);
/* Copy the completed frame into *out and atomically release the RX buffer.
   Returns BBP_OK, or BBP_ERR_NO_PACKET if none is ready. */
bbp_err_t bbp_poll(bbp_t *b, bbp_packet_t *out);

/* ---- TX ---- */
/* Build a frame into the TX buffer (SRC = self_addr). Does not transmit. */
bbp_err_t bbp_encode(bbp_t *b, uint8_t dest,
                     const char *schema, uint8_t schema_len,
                     const uint8_t *payload, uint8_t payload_len);
/* Transmit the encoded frame, owning the half-duplex DE/~RE turnaround. */
bbp_err_t bbp_transmit(bbp_t *b);

/* ---- helpers ---- */
uint16_t bbp_crc16_ccitt(const uint8_t *data, size_t len);  /* init 0xFFFF */
bool     bbp_schema_eq(const bbp_packet_t *p, const char *s);
void     bbp_get_stats(const bbp_t *b, bbp_stats_t *out);

/* big-endian payload accessors */
void     bbp_put_u16(uint8_t *p, uint16_t v);
void     bbp_put_u32(uint8_t *p, uint32_t v);
uint16_t bbp_get_u16(const uint8_t *p);
uint32_t bbp_get_u32(const uint8_t *p);

#ifdef __cplusplus
}
#endif

#endif /* BBP_H */
