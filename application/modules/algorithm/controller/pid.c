#include "pid.h"

/* ── 内部辅助 ────────────────────────────────────────────────────
 *  对称限幅：|val| ≤ limit；limit <= 0 时不限幅
 * ────────────────────────────────────────────────────────────── */
static inline float _clamp(float val, float limit)
{
    if (limit <= 0.0f) return val;
    if (val >  limit)  return  limit;
    if (val < -limit)  return -limit;
    return val;
}

/* ════════════════════════════════════════════════════════════════ */

void PID_Reset(PID_TypeDef *PID)
{
    PID->Error[0]    = 0.0f;
    PID->Error[1]    = 0.0f;
    PID->Integral    = 0.0f;
    PID->Output      = 0.0f;
    PID->Output_Last = 0.0f;
}

/* ────────────────────────────────────────────────────────────────
 *  位置式 PID
 *  u(k) = Kp·e(k) + Ki·∑e + Kd·[e(k) - e(k-1)]
 * ──────────────────────────────────────────────────────────────── */
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    float err = TargetValue - CurrentValue;

    /* 死区 */
    if (PID->DeadZone > 0.0f && fabsf(err) <= PID->DeadZone)
        err = 0.0f;

    /* 积分（含积分分离）：EIS_Max=0 → 始终积分 */
    if (PID->EIS_Max <= 0.0f || fabsf(err) <= PID->EIS_Max)
        PID->Integral += err * PID->Ki;
    PID->Integral = _clamp(PID->Integral, PID->Integral_Max);

    /* 输出 */
    PID->Output = PID->Kp * err
                + PID->Integral
                + PID->Kd * (err - PID->Error[0]);
    PID->Output = _clamp(PID->Output, PID->Output_Max);

    PID->Error[0] = err;
    return PID->Output;
}

/* ────────────────────────────────────────────────────────────────
 *  增量式 PID
 *  Δu(k) = (Kp+Ki+Kd)·e(k) - (Kp+2Kd)·e(k-1) + Kd·e(k-2)
 *  u(k)  = u(k-1) + Δu(k)
 * ──────────────────────────────────────────────────────────────── */
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    float err = TargetValue - CurrentValue;

    /* 死区 */
    if (PID->DeadZone > 0.0f && fabsf(err) <= PID->DeadZone)
        err = 0.0f;

    /* 动态 Ki：积分分离 + 抗积分饱和（两者均满足才保留积分）
     *   EIS_Max=0  → 不分离
     *   EAIS_Max=0 → 不抗饱和 */
    float ki_eff = PID->Ki;
    if (PID->EIS_Max > 0.0f && fabsf(err) > PID->EIS_Max)
        ki_eff = 0.0f;
    if (PID->EAIS_Max > 0.0f &&
        ((PID->Output_Last >=  PID->EAIS_Max && err > 0.0f) ||
         (PID->Output_Last <= -PID->EAIS_Max && err < 0.0f)))
        ki_eff = 0.0f;

    /* 增量公式 */
    PID->Output = PID->Output_Last
                + (PID->Kp + ki_eff + PID->Kd) * err
                + (-PID->Kp - 2.0f * PID->Kd)  * PID->Error[0]
                + PID->Kd                        * PID->Error[1];
    PID->Output = _clamp(PID->Output, PID->Output_Max);

    PID->Error[1]    = PID->Error[0];
    PID->Error[0]    = err;
    PID->Output_Last = PID->Output;
    return PID->Output;
}
