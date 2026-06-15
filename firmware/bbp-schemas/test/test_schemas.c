/*
 * test_schemas.c — host unit tests for the common-schema codecs (BS-SPEC-300).
 *
 *   make test   (from firmware/bbp-schemas)
 */
#include "bbp_schemas.h"
#include <stdio.h>

static int failures;

#define CHECK(cond) do {                                              \
    if (!(cond)) {                                                    \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        failures++;                                                   \
    }                                                                 \
} while (0)

int main(void)
{
    uint8_t  buf[8];
    uint8_t  idx;
    uint16_t val;
    bool     pressed;
    int8_t   delta;

    printf("test_pot\n");
    CHECK(bbp_pot_encode(buf, 3, 0x1234) == BBP_POT_LEN);
    CHECK(buf[0] == 3 && buf[1] == 0x12 && buf[2] == 0x34);   /* big-endian */
    CHECK(bbp_pot_decode(buf, BBP_POT_LEN, &idx, &val) && idx == 3 && val == 0x1234);
    CHECK(bbp_pot_decode(buf, BBP_POT_LEN, NULL, NULL));      /* NULL-safe */
    CHECK(!bbp_pot_decode(buf, 2, &idx, &val));               /* wrong length */
    CHECK(!bbp_pot_decode(NULL, BBP_POT_LEN, &idx, &val));    /* NULL buffer */

    printf("test_button\n");
    CHECK(bbp_button_encode(buf, 1, true) == BBP_BUTTON_LEN && buf[0] == 1 && buf[1] == 1);
    CHECK(bbp_button_decode(buf, BBP_BUTTON_LEN, &idx, &pressed) && idx == 1 && pressed);
    CHECK(bbp_button_encode(buf, 2, false) == BBP_BUTTON_LEN && buf[1] == 0);
    CHECK(bbp_button_decode(buf, BBP_BUTTON_LEN, &idx, &pressed) && !pressed);

    printf("test_encoder\n");
    CHECK(bbp_encoder_encode(buf, 0, -3) == BBP_ENCODER_LEN);
    CHECK(bbp_encoder_decode(buf, BBP_ENCODER_LEN, &idx, &delta) && idx == 0 && delta == -3);
    CHECK(bbp_encoder_encode(buf, 5, 127) == BBP_ENCODER_LEN);
    CHECK(bbp_encoder_decode(buf, BBP_ENCODER_LEN, &idx, &delta) && idx == 5 && delta == 127);
    CHECK(bbp_encoder_encode(buf, 0, -128) == BBP_ENCODER_LEN);
    CHECK(bbp_encoder_decode(buf, BBP_ENCODER_LEN, NULL, &delta) && delta == -128);

    /* button and encoder share a length; decoders must not be told apart by
       length alone — callers dispatch on the schema name first */
    CHECK(BBP_BUTTON_LEN == BBP_ENCODER_LEN);

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
}
