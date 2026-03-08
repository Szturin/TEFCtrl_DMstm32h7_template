#include "pid.h"

/* ════════════════════════════════════════════════════════════════
 *  内部辅助（static inline，零运行时开销）
 * ════════════════════════════════════════════════════════════════ */

/**
 * @brief 目标值斜率限制：每帧最多向目标值步进 step
 *        step <= 0 时直接跳变
 */
static inline float _ramp(float prev, float target, float step)
{
    if (step <= 0.0f) return target;
    if (target > prev) return fminf(prev + step, target);
    if (target < prev) return fmaxf(prev - step, target);
    return prev;
}

/**
 * @brief 对称限幅：|val| ≤ limit
 *        limit <= 0 时不限幅
 */
static inline float _clamp(float val, float limit)
{
    if (limit <= 0.0f) return val;
    if (val >  limit)  return  limit;
    if (val < -limit)  return -limit;
    return val;
}

/* ════════════════════════════════════════════════════════════════
 *  公共接口
 * ════════════════════════════════════════════════════════════════ */

void PID_Reset(PID_TypeDef *PID)
{
    PID->Error[0]    = 0.0f;
    PID->Error[1]    = 0.0f;
    PID->Integral    = 0.0f;
    PID->Output      = 0.0f;
    PID->Output_Last = 0.0f;
    PID->TargetValue = 0.0f;
}

/* ────────────────────────────────────────────────────────────────
 *  位置式 PID
 *  u(k) = Kp·e(k) + Ki·∑e + Kd·[e(k)-e(k-1)]
 * ──────────────────────────────────────────────────────────────── */
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    /* 1. 目标值斜率限制 */
    PID->TargetValue = _ramp(PID->TargetValue, TargetValue, PID->SpeedStep);

    /* 2. 计算误差 */
    float err = PID->TargetValue - CurrentValue;

    /* 3. 死区 */
    if (PID->DeadZone > 0.0f && fabsf(err) <= PID->DeadZone)
        err = 0.0f;

    /* 4. 积分（含积分分离）
     *    EIS_Max=0 → 不分离，始终积分
     *    EIS_Max>0 → |err| > EIS_Max 时停止积分，防止大误差下积分爆炸 */
    if (PID->EIS_Max <= 0.0f || fabsf(err) <= PID->EIS_Max)
        PID->Integral += err * PID->Ki;

    /* 5. 积分限幅 */
    PID->Integral = _clamp(PID->Integral, PID->Integral_Max);

    /* 6. PID 输出（D 项差分：当前误差 - 上帧误差） */
    PID->Output = PID->Kp * err
                + PID->Integral
                + PID->Kd * (err - PID->Error[0]);

    /* 7. 输出限幅 */
    PID->Output = _clamp(PID->Output, PID->Output_Max);

    /* 8. 更新历史 */
    PID->Error[0] = err;   // e(k) → e(k-1) for next call

    return PID->Output;
}

/* ────────────────────────────────────────────────────────────────
 *  增量式 PID
 *  Δu(k) = (Kp+Ki+Kd)·e(k) - (Kp+2Kd)·e(k-1) + Kd·e(k-2)
 *  u(k)  = u(k-1) + Δu(k)
 *
 *  优点：无积分项累积，天然抗饱和；适合执行机构有物理限位场景
 * ──────────────────────────────────────────────────────────────── */
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    /* 1. 目标值斜率限制 */
    PID->TargetValue = _ramp(PID->TargetValue, TargetValue, PID->SpeedStep);

    /* 2. 计算误差 */
    float err = PID->TargetValue - CurrentValue;

    /* 3. 死区 */
    if (PID->DeadZone > 0.0f && fabsf(err) <= PID->DeadZone)
        err = 0.0f;

    /* 4. 动态 Ki（积分分离 + 抗积分饱和，两者均满足才保留积分）
     *    EIS_Max=0  → 不分离
     *    EAIS_Max=0 → 不抗饱和 */
    float ki_eff = PID->Ki;
    if (PID->EIS_Max > 0.0f && fabsf(err) > PID->EIS_Max)
        ki_eff = 0.0f;
    if (PID->EAIS_Max > 0.0f &&
        ((PID->Output_Last >=  PID->EAIS_Max && err > 0.0f) ||
         (PID->Output_Last <= -PID->EAIS_Max && err < 0.0f)))
        ki_eff = 0.0f;

    /* 5. 标准增量公式 */
    PID->Output = PID->Output_Last
                + (PID->Kp + ki_eff + PID->Kd) * err
                + (-PID->Kp - 2.0f * PID->Kd)  * PID->Error[0]  // -e(k-1)
                + PID->Kd                        * PID->Error[1]; // +e(k-2)

    /* 6. 输出限幅 */
    PID->Output = _clamp(PID->Output, PID->Output_Max);

    /* 7. 更新历史 */
    PID->Error[1]    = PID->Error[0]; // e(k-1) → e(k-2)
    PID->Error[0]    = err;            // e(k)   → e(k-1)
    PID->Output_Last = PID->Output;

    return PID->Output;
}
