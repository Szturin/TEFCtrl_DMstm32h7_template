#include "vofa.h"
#include "usbd_cdc_if.h"
#include <string.h>

static const uint8_t VOFA_TAIL[4] = {0x00, 0x00, 0x80, 0x7F};

void vofa_justfloat_send(float *data, uint8_t ch_count) {
    if (ch_count > VOFA_MAX_CH) ch_count = VOFA_MAX_CH;
    uint8_t buf[VOFA_MAX_CH * 4 + 4];
    uint16_t len = ch_count * sizeof(float);
    memcpy(buf, data, len);
    memcpy(buf + len, VOFA_TAIL, 4);
    CDC_Transmit_HS(buf, len + 4);
}
