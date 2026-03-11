#include "vofa.h"
#include "usbd_cdc_if.h"
#include <string.h>

static const uint8_t VOFA_TAIL[4] = {0x00, 0x00, 0x80, 0x7F};

// ---- 接收处理 ----
static vofa_cmd_handler_t cmd_handler = NULL;
static char rx_line[64];       // 行缓冲（攒到 \n 才处理）
static uint8_t rx_pos = 0;

// USB CDC 接收回调（中断上下文，尽快返回）
static void vofa_cdc_rx(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        char c = (char)buf[i];
        if (c == '\n' || c == '\r') {
            if (rx_pos > 0) {
                rx_line[rx_pos] = '\0';
                if (cmd_handler) {
                    cmd_handler(rx_line);
                }
                rx_pos = 0;
            }
        } else if (rx_pos < sizeof(rx_line) - 1) {
            rx_line[rx_pos++] = c;
        }
    }
}

void vofa_init(void)
{
    rx_pos = 0;
    CDC_RegisterRxCallback(vofa_cdc_rx);
}

void vofa_register_cmd_handler(vofa_cmd_handler_t handler)
{
    cmd_handler = handler;
}

// ---- 发送 ----
void vofa_justfloat_send(float *data, uint8_t ch_count)
{
    if (ch_count > VOFA_MAX_CH) ch_count = VOFA_MAX_CH;
    uint8_t buf[VOFA_MAX_CH * 4 + 4];
    uint16_t len = ch_count * sizeof(float);
    memcpy(buf, data, len);
    memcpy(buf + len, VOFA_TAIL, 4);
    CDC_Transmit_HS(buf, len + 4);  // 非阻塞，忙则丢帧
}
