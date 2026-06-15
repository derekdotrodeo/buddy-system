/*
 * test_bbp.c — self-contained tests for the BBP core.
 *
 * Pure-software: a loopback platform captures transmitted bytes, which are then
 * fed back through the receive assembler. No hardware, no threads.
 *
 *   make test   (from firmware/bbp-core)
 */
#include "bbp.h"
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond) do {                                              \
    if (!(cond)) {                                                    \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        failures++;                                                   \
    }                                                                 \
} while (0)

/* ---- loopback platform: tx() appends to a capture buffer ---- */
typedef struct {
    uint8_t buf[BBP_MAX_FRAME * 4];
    size_t  len;
    int     de_asserts;     /* times set_driver_enable(true) was called */
    bool    de_state;
} loop_ctx_t;

static void loop_tx(void *ctx, const uint8_t *buf, size_t len)
{
    loop_ctx_t *c = (loop_ctx_t *)ctx;
    /* Driver must be enabled while bytes are on the wire (half-duplex). */
    CHECK(c->de_state == true);
    if (c->len + len <= sizeof c->buf) {
        memcpy(c->buf + c->len, buf, len);
        c->len += len;
    }
}

static void loop_de(void *ctx, bool on)
{
    loop_ctx_t *c = (loop_ctx_t *)ctx;
    if (on) c->de_asserts++;
    c->de_state = on;
}

/* Feed every captured byte through the receiver. */
static void feed(bbp_t *b, const loop_ctx_t *c)
{
    size_t i;
    for (i = 0; i < c->len; i++) {
        bbp_receive_byte(b, c->buf[i]);
    }
}

/* ---- 1. CRC canonical check value ---- */
static void test_crc_check_value(void)
{
    printf("test_crc_check_value\n");
    CHECK(bbp_crc16_ccitt((const uint8_t *)"123456789", 9) == 0x29B1u);
    CHECK(bbp_crc16_ccitt(NULL, 0) == 0xFFFFu);   /* init value, defensive */
}

/* ---- 2. encode -> wire -> receive round-trip ---- */
static void test_roundtrip(void)
{
    loop_ctx_t lc;
    bbp_platform_t plat;
    bbp_t tx, rx;
    bbp_packet_t pkt;
    const uint8_t payload[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    printf("test_roundtrip\n");
    memset(&lc, 0, sizeof lc);
    memset(&plat, 0, sizeof plat);
    plat.tx = loop_tx;
    plat.set_driver_enable = loop_de;
    plat.ctx = &lc;

    CHECK(bbp_init(&tx, &plat, 3) == BBP_OK);          /* SRC = slot 3 */
    CHECK(bbp_init(&rx, &plat, BBP_HOST_ADDR) == BBP_OK);

    CHECK(bbp_encode(&tx, BBP_HOST_ADDR, "bbp.info", 8,
                     payload, sizeof payload) == BBP_OK);
    CHECK(bbp_transmit(&tx) == BBP_OK);
    CHECK(lc.de_asserts == 1);          /* one turnaround */
    CHECK(lc.de_state == false);        /* DE dropped after tx */

    feed(&rx, &lc);
    CHECK(bbp_poll(&rx, &pkt) == BBP_OK);
    CHECK(pkt.src == 3);
    CHECK(pkt.dest == BBP_HOST_ADDR);
    CHECK(pkt.schema_len == 8);
    CHECK(bbp_schema_eq(&pkt, "bbp.info"));
    CHECK(pkt.payload_len == sizeof payload);
    CHECK(memcmp(pkt.payload, payload, sizeof payload) == 0);
    CHECK(bbp_poll(&rx, &pkt) == BBP_ERR_NO_PACKET);   /* drained */

    bbp_stats_t st;
    bbp_get_stats(&rx, &st);
    CHECK(st.rx_good == 1);
    CHECK(st.rx_crc_err == 0);
    CHECK(st.rx_frame_err == 0);
}

/* ---- 3. empty schema + empty payload (minimal frame) ---- */
static void test_empty_fields(void)
{
    loop_ctx_t lc;
    bbp_platform_t plat;
    bbp_t b;
    bbp_packet_t pkt;

    printf("test_empty_fields\n");
    memset(&lc, 0, sizeof lc);
    memset(&plat, 0, sizeof plat);
    plat.tx = loop_tx;
    plat.set_driver_enable = loop_de;
    plat.ctx = &lc;

    CHECK(bbp_init(&b, &plat, 1) == BBP_OK);
    CHECK(bbp_encode(&b, 2, NULL, 0, NULL, 0) == BBP_OK);
    CHECK(bbp_transmit(&b) == BBP_OK);
    CHECK(lc.len == BBP_MIN_FRAME);     /* SOF+SRC+DEST+0+0+CRC = 7, captured on tx */
    feed(&b, &lc);
    CHECK(bbp_poll(&b, &pkt) == BBP_OK);
    CHECK(pkt.schema_len == 0 && pkt.payload_len == 0);
}

/* ---- 4. corrupted CRC is rejected ---- */
static void test_crc_reject(void)
{
    loop_ctx_t lc;
    bbp_platform_t plat;
    bbp_t b;
    bbp_packet_t pkt;
    bbp_stats_t st;

    printf("test_crc_reject\n");
    memset(&lc, 0, sizeof lc);
    memset(&plat, 0, sizeof plat);
    plat.tx = loop_tx;
    plat.set_driver_enable = loop_de;
    plat.ctx = &lc;

    CHECK(bbp_init(&b, &plat, 1) == BBP_OK);
    CHECK(bbp_encode(&b, 2, "x", 1, (const uint8_t *)"y", 1) == BBP_OK);
    CHECK(bbp_transmit(&b) == BBP_OK);
    lc.buf[lc.len - 1] ^= 0xFF;          /* flip the last CRC byte */
    feed(&b, &lc);
    CHECK(bbp_poll(&b, &pkt) == BBP_ERR_NO_PACKET);
    bbp_get_stats(&b, &st);
    CHECK(st.rx_crc_err == 1);
    CHECK(st.rx_good == 0);
}

/* ---- 5. resync: leading garbage + a stray SOF before a good frame ---- */
static void test_resync(void)
{
    loop_ctx_t lc;
    bbp_platform_t plat;
    bbp_t b;
    bbp_packet_t pkt;

    printf("test_resync\n");
    memset(&lc, 0, sizeof lc);
    memset(&plat, 0, sizeof plat);
    plat.tx = loop_tx;
    plat.set_driver_enable = loop_de;
    plat.ctx = &lc;

    CHECK(bbp_init(&b, &plat, 4) == BBP_OK);

    /* junk, including a 0x7E that opens a frame with an invalid SRC */
    bbp_receive_byte(&b, 0x00);
    bbp_receive_byte(&b, 0xFF);
    bbp_receive_byte(&b, BBP_SOF);
    bbp_receive_byte(&b, 0xAA);   /* invalid SRC -> frame_err, reprocess as HUNT */

    CHECK(bbp_encode(&b, BBP_HOST_ADDR, "p", 1, (const uint8_t *)"q", 1) == BBP_OK);
    CHECK(bbp_transmit(&b) == BBP_OK);
    feed(&b, &lc);

    CHECK(bbp_poll(&b, &pkt) == BBP_OK);
    CHECK(pkt.src == 4);
    CHECK(bbp_schema_eq(&pkt, "p"));
}

/* ---- 6. encode argument validation ---- */
static void test_encode_validation(void)
{
    bbp_platform_t plat;
    bbp_t b;
    uint8_t big[BBP_MAX_SCHEMA + 1] = {0};

    printf("test_encode_validation\n");
    memset(&plat, 0, sizeof plat);
    plat.tx = loop_tx;          /* required by init */
    CHECK(bbp_init(&b, &plat, 1) == BBP_OK);

    CHECK(bbp_encode(NULL, 2, NULL, 0, NULL, 0) == BBP_ERR_NULL_ARG);
    CHECK(bbp_encode(&b, 2, NULL, 5, NULL, 0) == BBP_ERR_NULL_ARG);   /* len>0, ptr NULL */
    CHECK(bbp_encode(&b, 2, (const char *)big, BBP_MAX_SCHEMA + 1,
                     NULL, 0) == BBP_ERR_SCHEMA_TOO_LONG);
    CHECK(bbp_encode(&b, 0x09 /* bad slot */, "a", 1, NULL, 0) == BBP_ERR_INVALID_ADDR);
}

/* ---- 7. big-endian accessors ---- */
static void test_accessors(void)
{
    uint8_t buf[4];
    printf("test_accessors\n");
    bbp_put_u16(buf, 0x1234);
    CHECK(buf[0] == 0x12 && buf[1] == 0x34);
    CHECK(bbp_get_u16(buf) == 0x1234);
    bbp_put_u32(buf, 0xDEADBEEFu);
    CHECK(buf[0] == 0xDE && buf[3] == 0xEF);
    CHECK(bbp_get_u32(buf) == 0xDEADBEEFu);
}

int main(void)
{
    test_crc_check_value();
    test_roundtrip();
    test_empty_fields();
    test_crc_reject();
    test_resync();
    test_encode_validation();
    test_accessors();

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
}
