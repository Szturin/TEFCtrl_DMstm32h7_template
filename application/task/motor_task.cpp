#include "motor_task.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include "motor/dm_motor/dm_motor.h"
#include "motor/dji_motor/dji_motor.h"
#include "algorithm/controller/pid.h"
#include "vofa+/vofa.h"
#include "usbd_cdc_if.h"

extern Uart_Instance uart_log;

// DM 电机实例（工程机器人两轴）
static DMMotor motor1({
    .motor_id = 0x01,
    .set_mode = DMMotor::POS_MODE,
    .hcan     = &hfdcan1,
    .p_max    = 12.5f,
    .v_max    = 500.0f,
    .t_max    = 10.0f
});

static DMMotor motor2({
    .motor_id = 0x02,
    .set_mode = DMMotor::POS_MODE,
    .hcan     = &hfdcan1,
    .p_max    = 12.5f,
    .v_max    = 500.0f,
    .t_max    = 10.0f
});

// 电机任务初始化
void motor_task_init(void) {
    HAL_Delay(800);
    //motor1.enable();  // DEBUG: 暂停DM电机，排除CAN干扰
    HAL_Delay(300);
    //motor2.enable();
    HAL_Delay(300);
    uart_log.printf("Motor enabled.\r\n");
}

// 电机任务处理函数 (5ms周期)
void motor_task_proc(void) {
    //motor1.setPosition(10.0, 8.0);  // DEBUG: 暂停DM电机
    //motor2.setPosition(5.0, 8.0);
}

// ==============================================================================
// 发射机构（M2006 拨蛋盘 + C615 蜗牛电机）
// ==============================================================================
static DJIMotor shoot_motor({
    .type     = DJIMotor::Type::M2006,
    .hfdcan   = &hfdcan1,
    .motor_id = 1
});

// 电机轴rpm → 输出轴rad/s（减速比36:1）
#define RPM_TO_RADS  (2.0f * 3.14159265f / 60.0f / 36.0f)

static PID_TypeDef speed_pid = {
    .Kp = 500.0f,
    .Ki = 0.0f,
    .Kd = 0.0f,
    .FF_gain = 0.0f,
    .D_alpha = 0.0f,
    .Output_Max = 10000.0f,
    .Integral_Max = 5000.0f,
    .DeadZone = 0.0f,
    .EIS_Max = 0.0f,
    .EAIS_Max = 0.0f,
};

static float target_rads = 1.5f;  // 输出轴目标角速度 rad/s（约86rpm）

void shoot_task_init(void) {
    shoot_motor.enable();
}

void shoot_task_proc(void) {
    // 1. 电机轴rpm → 输出轴 rad/s
    float actual_rads = shoot_motor.getFeedback().speed_rpm * RPM_TO_RADS;

    // 2. PID 计算
    float output = Position_PID(&speed_pid, actual_rads, target_rads);

    // 3. 设置电流并发送
    shoot_motor.setCurrent((int16_t)output);
    DJIMotor::sendAll();

    // 4. VOFA+ 波形（3通道：目标rad/s, 实际rad/s, PID输出）
    float vofa_data[3] = { target_rads, actual_rads, output };
    vofa_justfloat_send(vofa_data, 3);
}