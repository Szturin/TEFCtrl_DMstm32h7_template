#ifndef REMOTER_UART_H
#define REMOTER_UART_H

#include "bsp_system.h"
#define WBUS_BUFLEN    (25)

typedef struct {
    struct {
        int16_t ch1;   // 遥控通道1
        int16_t ch2;   // 遥控通道2
        int16_t ch3;   // 遥控通道3
        int16_t ch4;   // 遥控通道4

        int16_t SA;    // 拨片通道1
        int16_t SB;    // 拨片通道2
        int16_t SC;    // 拨片通道3
        int16_t SD;    // 拨片通道4
        int16_t SE;    // 拨片通道5
        int16_t SF;    // 拨片通道6
        int16_t SG;    // 拨片通道7
        int16_t SH;    // 拨片通道8

        int16_t LD;    // 旋钮通道1
        int16_t RD;    // 旋钮通道2
        int16_t LS;    // 拨轮通道1
        int16_t RS;    // 拨轮通道2

    } remote;
} wbus_rc_info_t;

extern wbus_rc_info_t wbus_rc;

void RemoteInit(void);
void parse_wbus_data(wbus_rc_info_t *rc_wbus, uint8_t *buff);

#endif


