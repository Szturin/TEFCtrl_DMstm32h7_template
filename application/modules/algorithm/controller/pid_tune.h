#ifndef CONTROLLER_PID_TUNE_H
#define CONTROLLER_PID_TUNE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_link.h"

// VOFA+ 串口滑块调参用结构体
typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float Ks;
} PID_Params_t;

extern PID_Params_t PID_Test;

// 解析 VOFA+ 发来的串口命令，格式 "Kp:1.5"
void PID_UpdateFromCommand(PID_Params_t *pid, const char *command);

// 将当前参数通过串口打印（VOFA+ 观测用）
void PID_PrintParams(UART_HandleTypeDef *huart, PID_Params_t *pid);

// @todo 前馈角度控制，待实现后移入 feedforward.c
// float feedforward_control_angle(float target, float target_last, float K);

#ifdef __cplusplus
}
#endif

#endif // CONTROLLER_PID_TUNE_H
