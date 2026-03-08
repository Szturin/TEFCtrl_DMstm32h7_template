#ifndef CONTROLLER_PID_H
#define CONTROLLER_PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_link.h"

/**
 * @brief PID 控制器参数与状态结构体
 *
 * 约定：限幅字段值为 0 时表示"不启用"该功能
 *   DeadZone=0    → 无死区
 *   EIS_Max=0     → 不启用积分分离
 *   EAIS_Max=0    → 不启用抗积分饱和
 *   Integral_Max=0 → 积分不限幅
 *   Output_Max=0  → 输出不限幅
 *   SpeedStep=0   → 目标值直跳，不做斜率限制
 */
typedef struct __PID_TypeDef
{
    /* ── 调参区（用户设置） ─────────────────────────────────── */
    float Kp;
    float Ki;
    float Kd;

    float Output_Max;    // 输出限幅，对称 ±
    float Integral_Max;  // 积分限幅，对称 ±（位置式）
    float DeadZone;      // 误差死区
    float EIS_Max;       // 积分分离阈值：|err|>EIS_Max 时停止积分
    float EAIS_Max;      // 抗积分饱和阈值：输出饱和且同向时停止积分（增量式）
    float SpeedStep;     // 目标值每帧最大步进（斜率限制）

    /* ── 内部状态（勿手动修改） ─────────────────────────────── */
    float Error[2];      // [0]=e(k-1)  [1]=e(k-2)
    float Integral;      // 积分累积（位置式）
    float Output;        // 本帧输出
    float Output_Last;   // 上帧输出（增量式）
    float TargetValue;   // 当前斜率目标（SpeedStep 使用）

    /* ── 可选函数指针（灵活调用） ───────────────────────────── */
    float (*Calc)(struct __PID_TypeDef *pid, float current, float target);
    void  (*RST) (struct __PID_TypeDef *pid);

} PID_TypeDef;


/**
 * @brief 清零所有内部状态（不改变参数）
 */
void PID_Reset(PID_TypeDef *PID);

/**
 * @brief 位置式 PID
 *        u = Kp*e + Ki*∫e + Kd*Δe
 */
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue);

/**
 * @brief 增量式 PID
 *        Δu = (Kp+Ki+Kd)*e[k] - (Kp+2Kd)*e[k-1] + Kd*e[k-2]
 */
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue);

#ifdef __cplusplus
}
#endif

#endif // CONTROLLER_PID_H
