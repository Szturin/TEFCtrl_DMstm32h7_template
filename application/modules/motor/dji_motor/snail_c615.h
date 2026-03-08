//
// Created by 123 on 2026/1/17.
//

#ifndef SNAIL_C615_H
#define SNAIL_C615_H
#include "bsp_system.h"

#ifdef __cplusplus
extern "C" { // 存放C接口
#endif
void snail_init();
void snail_set_speed(uint16_t speed);
void snail_stop(void);
void snail_fast_control(uint16_t target);

#ifdef __cplusplus
}
#endif


#endif //CTRBOARD_H7_ALL_SNAIL_C615_H
