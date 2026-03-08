#include "pid.h"

void PID_Reset(PID_TypeDef *PID)
{
    PID->Error[0] = PID->Error[1] = PID->Error[2] = 0;
    PID->Integral = 0;
    PID->Output = 0;
    PID->Output_Last = 0;
}

// 位置式PID
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    if (PID->SpeedStep != 0) {
        if (TargetValue > PID->TargetValue) {
            PID->TargetValue += PID->SpeedStep;
            if (PID->TargetValue > TargetValue) PID->TargetValue = TargetValue;
        } else if (TargetValue < PID->TargetValue) {
            PID->TargetValue -= PID->SpeedStep;
            if (PID->TargetValue < TargetValue) PID->TargetValue = TargetValue;
        }
    } else {
        PID->TargetValue = TargetValue;
    }

    PID->Error[0] = PID->TargetValue - CurrentValue;

    // 死区
    if ((PID->Error[0] <= PID->DeadZone) && (PID->Error[0] >= -PID->DeadZone))
        PID->Error[0] = 0.0f;

    // 积分分离：误差超出 EIS_Max 时停止积分
    if ((PID->EIS_Max == 0) || ((PID->Error[0] <= PID->EIS_Max) && (PID->Error[0] >= -PID->EIS_Max)))
        PID->Integral += PID->Error[0] * PID->Ki;

    // 积分限幅
    if (PID->Integral > PID->Integral_Max)       PID->Integral =  PID->Integral_Max;
    else if (PID->Integral < -PID->Integral_Max) PID->Integral = -PID->Integral_Max;

    PID->Output = PID->Kp * PID->Error[0] + PID->Integral + PID->Kd * (PID->Error[0] - PID->Error[1]);
    PID->Error[1] = PID->Error[0];

    // 输出限幅
    if (PID->Output >  PID->Output_Max) PID->Output =  PID->Output_Max;
    if (PID->Output < -PID->Output_Max) PID->Output = -PID->Output_Max;

    return PID->Output;
}

// 增量式PID
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    float A0, A1 = -PID->Kp - 2 * PID->Kd, A2 = PID->Kd;
    char KL = 1;

    if (PID->SpeedStep != 0) {
        if (TargetValue > PID->TargetValue) {
            PID->TargetValue += PID->SpeedStep;
            if (PID->TargetValue > TargetValue) PID->TargetValue = TargetValue;
        } else if (TargetValue < PID->TargetValue) {
            PID->TargetValue -= PID->SpeedStep;
            if (PID->TargetValue < TargetValue) PID->TargetValue = TargetValue;
        }
    } else {
        PID->TargetValue = TargetValue;
    }

    PID->Error[0] = PID->TargetValue - CurrentValue;

    // 死区
    if ((PID->Error[0] <= PID->DeadZone) && (PID->Error[0] >= -PID->DeadZone))
        PID->Error[0] = 0.0f;

    // 积分分离
    if (!((PID->Error[0] <= PID->EIS_Max) && (PID->Error[0] >= -PID->EIS_Max)))
        KL = 0;

    // 抗积分饱和
    if ((PID->Output_Last >= PID->EAIS_Max && PID->Error[0] >= 0) ||
        (PID->Output_Last <= -PID->EAIS_Max && PID->Error[0] <= 0))
        KL = 0;

    A0 = PID->Kp + PID->Kd + (PID->Ki * KL);

    PID->Output = PID->Output_Last + A0 * PID->Error[0] + A1 * PID->Error[1] + A2 * PID->Error[2];

    if (PID->Output >  PID->Output_Max) PID->Output =  PID->Output_Max;
    if (PID->Output < -PID->Output_Max) PID->Output = -PID->Output_Max;

    PID->Error[2] = PID->Error[1];
    PID->Error[1] = PID->Error[0];
    PID->Output_Last = PID->Output;

    return PID->Output;
}

/* 前馈控制角度，待实现 */
float feedforward_control_angle(float target, float target_last, float K)
{
    (void)target; (void)target_last; (void)K;
    return 0.0f;
}
