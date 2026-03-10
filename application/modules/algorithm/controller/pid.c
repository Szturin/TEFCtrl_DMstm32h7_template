#include "pid.h"

/* ── 内部辅助 ─────────────────────────────────────────────────── */

/** 对称限幅：|val| ≤ limit；limit <= 0 时不限幅 */
static inline float _clamp(float val, float limit)
{
    if (limit <= 0.0f) return val;
    if (val >  limit)  return  limit;
    if (val < -limit)  return -limit;
    return val;
}

/** 一阶低通滤波：alpha=0 直通，alpha→1 滤波越强 */
static inline float _lpf(float prev, float raw, float alpha)
{
    return alpha * prev + (1.0f - alpha) * raw;
}

/* ════════════════════════════════════════════════════════════════ */

void PID_Reset(PID_TypeDef *PID)
{
    PID->Error[0]     = 0.0f;
    PID->Error[1]     = 0.0f;
    PID->Integral     = 0.0f;
    PID->D_filter     = 0.0f;
    PID->Feedback_Last= 0.0f;
    PID->Output       = 0.0f;
    PID->Output_Last  = 0.0f;
}

/* ────────────────────────────────────────────────────────────────
 *  位置式 PID
 *
 *  优化项：
 *  1. 微分先行  —— D 项作用于反馈量而非误差，避免 setpoint 阶跃时微分跳变
 *  2. 微分滤波  —— 一阶低通抑制传感器高频噪声对 D 项的影响
 *  3. 限幅反饱和 —— 输出已饱和且误差方向相同时，停止积分（精确抗积分饱和）
 *  4. 前馈      —— output += FF_gain * target，改善跟踪速度
 * ──────────────────────────────────────────────────────────────── */
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    float err = TargetValue - CurrentValue;

    /* 1. 死区 */
    if (PID->DeadZone > 0.0f && fabsf(err) <= PID->DeadZone)
        err = 0.0f;

    /* 2. 限幅反饱和 + 积分分离 → 决定本帧是否积分
     *    饱和条件：输出已达上限且误差仍往同方向推
     *    分离条件：|err| > EIS_Max（EIS_Max=0 则不分离） */
    uint8_t saturated = (PID->Output_Max > 0.0f) &&
                        ((PID->Output >=  PID->Output_Max && err > 0.0f) ||
                         (PID->Output <= -PID->Output_Max && err < 0.0f));
    uint8_t separated = (PID->EIS_Max > 0.0f) && (fabsf(err) > PID->EIS_Max);

    if (!saturated && !separated)
        PID->Integral += err * PID->Ki;
    PID->Integral = _clamp(PID->Integral, PID->Integral_Max);

    /* 3. 微分先行：对反馈量求差分（setpoint 变化时 D 项不跳变）
     *    D_raw = -(y[k] - y[k-1]) = prev_feedback - current_feedback */
    float D_raw = PID->Feedback_Last - CurrentValue;

    /* 4. 微分滤波：一阶低通（D_alpha=0 → 直通，无滤波） */
    PID->D_filter = _lpf(PID->D_filter, D_raw, PID->D_alpha);

    /* 5. 合成输出：前馈 + PID */
    PID->Output = PID->FF_gain * TargetValue
                + PID->Kp * err
                + PID->Integral
                + PID->Kd * PID->D_filter;
    PID->Output = _clamp(PID->Output, PID->Output_Max);

    /* 6. 更新历史 */
    PID->Error[0]      = err;
    PID->Feedback_Last = CurrentValue;

    return PID->Output;
}

/* ────────────────────────────────────────────────────────────────
 *  增量式 PID
 *
 *  优化项：
 *  1. 微分滤波  —— 对二阶差分（D 项）做低通滤波
 *  2. 积分分离  —— EIS_Max 控制
 *  3. 抗积分饱和 —— EAIS_Max 控制（Output_Last 饱和时停止积分）
 *  4. 前馈      —— 叠加到本帧增量
 *
 *  增量式天然抗积分饱和（Output_Last 每帧限幅），EAIS 进一步增强
 *
 *  标准公式：
 *  Δu(k) = (Kp+ki+Kd)·e[k] - (Kp+2Kd)·e[k-1] + Kd·e[k-2]
 * ──────────────────────────────────────────────────────────────── */
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    float err = TargetValue - CurrentValue;

    /* 1. 死区 */
    if (PID->DeadZone > 0.0f && fabsf(err) <= PID->DeadZone)
        err = 0.0f;

    /* 2. 动态 Ki（积分分离 + 抗积分饱和） */
    float ki_eff = PID->Ki;
    if (PID->EIS_Max > 0.0f && fabsf(err) > PID->EIS_Max)
        ki_eff = 0.0f;
    if (PID->EAIS_Max > 0.0f &&
        ((PID->Output_Last >=  PID->EAIS_Max && err > 0.0f) ||
         (PID->Output_Last <= -PID->EAIS_Max && err < 0.0f)))
        ki_eff = 0.0f;

    /* 3. 微分项（二阶差分）+ 低通滤波 */
    float D_raw   = err - 2.0f * PID->Error[0] + PID->Error[1];
    PID->D_filter = _lpf(PID->D_filter, D_raw, PID->D_alpha);

    /* 4. 增量公式（D 项替换为滤波后结果） */
    float delta = PID->Kp * (err - PID->Error[0])  // ΔP
                + ki_eff  * err                     // ΔI
                + PID->Kd * PID->D_filter;          // ΔD（已滤波）

    /* 5. 前馈叠加到总输出（而非增量，避免前馈被增量积累放大） */
    PID->Output = _clamp(PID->Output_Last + delta, PID->Output_Max)
                + PID->FF_gain * TargetValue;
    PID->Output = _clamp(PID->Output, PID->Output_Max);

    /* 6. 更新历史 */
    PID->Error[1]    = PID->Error[0];
    PID->Error[0]    = err;
    PID->Output_Last = _clamp(PID->Output_Last + delta, PID->Output_Max);

    return PID->Output;
}
