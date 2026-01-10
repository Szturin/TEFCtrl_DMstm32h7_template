#include "pid.h"

void PID_Reset(PID_TypeDef *PID)
{
    PID->Error[0] = PID->Error[1] = PID->Error[2] = 0;
    PID->Integral = 0;

    PID->Output = 0;
    PID->Output_Last = 0;
}

//位置式PID
float Position_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{

    if(PID->SpeedStep != 0){
        if (TargetValue > PID->TargetValue) {
            PID->TargetValue += PID->SpeedStep;
            if (PID->TargetValue > TargetValue) {
                PID->TargetValue = TargetValue;
            }
        } else if (TargetValue < PID->TargetValue) {
            PID->TargetValue -= PID->SpeedStep;
            if (PID->TargetValue < TargetValue) {
                PID->TargetValue = TargetValue;
            }
        }
    }else{
        PID->TargetValue = TargetValue;
    }

    PID->Error[0] = PID->TargetValue - CurrentValue;

    /************************根据偏差对PID参数进行调整***************************/
    //死区计算，设为0则取消效果
    if ((PID->Error[0] <= PID->DeadZone) && (PID->Error[0] >= -PID->DeadZone)) //在死区内
    {
        PID->Error[0] = 0.0f;
    }

    //积分分离，误差值超过限制则取消积分效果，防止超调
    if ((PID->EIS_Max == 0) || ((PID->Error[0] <= PID->EIS_Max) && (PID->Error[0] >= -PID->EIS_Max)))
    {
        PID->Integral += PID->Error[0] * PID->Ki;
    }

    //积分限幅，设为一个足够大的数则取消效果
    if (PID->Integral >= PID->Integral_Max)
    {
        PID->Integral = PID->Integral_Max;
    }
    else if (PID->Integral <= -PID->Integral_Max)
    {
        PID->Integral = -PID->Integral_Max;
    }

    /********************************PID计算***************************************/
    PID->Output = PID->Kp * PID->Error[0] + PID->Integral + PID->Kd * (PID->Error[0] - PID->Error[1]);

    /****************************为下次PID计算做准备********************************/
    PID->Error[1] = PID->Error[0];

    /************************************输出限幅***********************************/
    if (PID->Output >= PID->Output_Max)
    {
        PID->Output = PID->Output_Max;
    }
    else if (PID->Output <= -PID->Output_Max)
    {
        PID->Output = -PID->Output_Max;
    }

    return PID->Output;
}

//增量式PID
float Incremental_PID(PID_TypeDef *PID, float CurrentValue, float TargetValue)
{
    float A0 = PID->Kp + PID->Kd;
    float A1 = -PID->Kp - 2 * PID->Kd;
    float A2 = PID->Kd;
    char KL = 1;

    // 阶梯变速控制：逐步逼近目标值

    if(PID->SpeedStep != 0){
        if (TargetValue > PID->TargetValue) {
            PID->TargetValue += PID->SpeedStep;
            if (PID->TargetValue > TargetValue) {
                PID->TargetValue = TargetValue;
            }
        } else if (TargetValue < PID->TargetValue) {
            PID->TargetValue -= PID->SpeedStep;
            if (PID->TargetValue < TargetValue) {
                PID->TargetValue = TargetValue;
            }
        }
    }else{
        PID->TargetValue = TargetValue;
    }


    PID->Error[0] = PID->TargetValue - CurrentValue;

    /************************根据偏差对PID参数进行调整***************************/
    // 死区计算，设为0则取消效果
    if ((PID->Error[0] <= PID->DeadZone) && (PID->Error[0] >= -PID->DeadZone)) {
        PID->Error[0] = 0.0;
    }

    // 积分分离，超过积分分离限幅取消效果
    if ((PID->Error[0] <= PID->EIS_Max) && (PID->Error[0] >= -PID->EIS_Max)) {
        KL = 1;
    } else {
        KL = 0;
    }

    // 抗积分饱和，设为一个足够大的数则取消效果
    if ((PID->Output_Last >= PID->EAIS_Max && PID->Error[0] >= 0) || (PID->Output_Last <= -PID->EAIS_Max && PID->Error[0] <= 0)) {
        KL = 0;
    }

    // 计算最终A0，包含积分的影响
    A0 = PID->Kp + PID->Kd + (PID->Ki * KL);

    /********************************PID计算***************************************/
    PID->Output = PID->Output_Last + A0 * PID->Error[0] + A1 * PID->Error[1] + A2 * PID->Error[2];

    /************************************输出限幅***********************************/
    if (PID->Output >= PID->Output_Max) {
        PID->Output = PID->Output_Max;
    } else if (PID->Output <= -PID->Output_Max) {
        PID->Output = -PID->Output_Max;
    }

    /****************************为下次PID计算做准备********************************/
    PID->Error[2] = PID->Error[1];
    PID->Error[1] = PID->Error[0];
    PID->Output_Last = PID->Output;

    return PID->Output;
}

/* 前馈（角度）计算 */
float feedforward_control_angle(float target,float target_last,float K)
{
    //float feed_value = K * error_angle_calc(target ,target_last);//前馈，同理要计算最小差角，否则一定会出现角度突变的情况
    //return feed_value;
}