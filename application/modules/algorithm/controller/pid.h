#ifndef CONTROLLER_PID_H
#define CONTROLLER_PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_link.h"

typedef struct __PID_TypeDef
{
    float Kp;
    float Ki;
    float Kd;

    float Output_Max;       // 输出限幅
    float DeadZone;         // 死区，为0则取消
    float EIS_Max;          // 积分分离阈值（位置式），为0则取消
    float EAIS_Max;         // 抗积分饱和阈值（增量式），为0则取消
    float Integral_Max;     // 积分限幅（位置式），为0则取消

    float Error[3];         // 0当前 1上次 2上上次
    float Integral;

    float Output;
    float Output_Last;      // 增量式使用
    float SpeedStep;        // 目标值速率限制，为0则取消

    float TargetValue;

    float (*Calc)(struct __PID_TypeDef *pid, float Input, float Setpoint);
    void  (*RST) (struct __PID_TypeDef *pid);
} PID_TypeDef;


void  PID_Reset(PID_TypeDef *PID);
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue);
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue);
float feedforward_control_angle(float target, float target_last, float K);

#ifdef __cplusplus
}
#endif

#endif // CONTROLLER_PID_H
