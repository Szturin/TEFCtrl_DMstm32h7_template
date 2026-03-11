//
// Created by 123 on 2026/3/11.
//

#ifndef CTRBOARD_H7_ALL_POWERMETER_H
#define CTRBOARD_H7_ALL_POWERMETER_H

#include "bsp/can/bsp_fdcan.h"
#ifdef __cplusplus
extern "C" {
#endif

// 定义功率计数据结构
typedef struct{
    // 物理测量值
    float power;
    float voltage; // (V)
    float current; // (A)

    // BSP实例指针
    FDCANInstance *can_ins;
} PowerMeter_t;

/**
 * @brief  初始化功率计
 * @param device
 */
PowerMeter_t* PowerMeter_Init(FDCAN_HandleTypeDef *hfdcan);
#ifdef __cplusplus
}
#endif
#endif //CTRBOARD_H7_ALL_POWERMETER_H