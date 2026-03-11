#include "motor_task.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include <math.h>
#include "motor/dm_motor/dm_motor.h"
#include "motor/dji_motor/dji_motor.h"
#include "motor/unitree_motor/unitree_motor.h"
#include "algorithm/controller/pid.h"
#include "vofa+/vofa.h"
#include "usbd_cdc_if.h"
#include "daemon/daemon.h"
#include "powermeter/powermeter.h"

PowerMeter_t *chassis_power_meter;

void Robot_Init(void) {
    // 2. 初始化（会自动完成 CAN 过滤器注册）
    chassis_power_meter = PowerMeter_Init(&hfdcan1);
}

void Chassis_Task(void) {
    // 3. 直接读取数据
    float current_v = chassis_power_meter->voltage;
    float current_p = chassis_power_meter->power;
float current_i = chassis_power_meter->current;

    // 应用于功率限制逻辑...
}


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
    .motor_id = 2
});

static DJIMotor motor_3508_1({
        .type     = DJIMotor::Type::M2006,
        .hfdcan   = &hfdcan1,
        .motor_id = 1
});

// 电机轴rpm → 输出轴rad/s（减速比36:1）
#define RPM_TO_RADS  (2.0f * 3.14159265f / 60.0f / 36.0f)

// 速度 PID
static PID_TypeDef speed_pid = {
    .Kp          = 400.0f,
    .Ki          = 1.0f,
    .Kd          = 0.0f,
    .FF_gain     = 2.0f,      // 静态前馈 1/Km
    .Output_Max  = 10000.0f,
    .Integral_Max = 2000.0f,
};

// 速度前馈增益 = τ/Km = 0.025/0.0617
static constexpr float FF_VEL = 9.45;

static constexpr float Ts = 0.001f;  // 1ms 速度环

// 正弦目标参数
static constexpr float SINE_CENTER = 15.0f;
static constexpr float SINE_AMP    = 10.0f;
static constexpr float SINE_FREQ   = 2.0f;   // 2Hz

static uint32_t tick_count = 0;
static float target_last = 0;  // 上次目标值（通用数值微分）

void shoot_task_init(void) {
    PID_Reset(&speed_pid);
    target_last = SINE_CENTER;  // 初始目标
    shoot_motor.enable();
}

void shoot_task_proc(void) {
    float t = tick_count * Ts;
    float target_rads = SINE_CENTER + SINE_AMP * sinf(2.0f * 3.14159265f * SINE_FREQ * t);

    // 速度前馈：数值微分 (target - target_last) / Ts
    // 目标值是自己生成的，天然平滑，数值微分不会有噪声问题
    float target_dot = (target_rads - target_last) / Ts;
    target_last = target_rads;

    tick_count++;

    // 反馈
    float actual_rads = shoot_motor.getFeedback().speed_rpm * RPM_TO_RADS;

    // 位置式PID（含静态前馈FF_gain * target）
    float output = Position_PID(&speed_pid, actual_rads, target_rads);

    // 叠加速度前馈，补偿相位滞后
    output += FF_VEL * target_dot;

    // 限幅
    if (output >  10000.0f) output =  10000.0f;
    if (output < -10000.0f) output = -10000.0f;

    // 电流输出
    shoot_motor.setCurrent((int16_t)output);
    DJIMotor::sendAll();

    // VOFA+ 降频输出（每5ms发一次）
    if ((tick_count % 5) == 0) {
        float vofa_data[5] = {
            target_rads,                 // CH0: 目标
            actual_rads,                 // CH1: 实际
            output,                      // CH2: PID输出
            target_rads - actual_rads,   // CH3: 误差
            FF_VEL * target_dot          // CH4: 速度前馈量
        };
        vofa_justfloat_send(vofa_data, 5);
    }
}

// ==============================================================================
// 宇树电机测试 (GO-M8010-6 / A1, RS485 via USART3)
// ==============================================================================
static UnitreeMotor unitree_motor({
    .type     = UnitreeMotor::GO_M8010_6,
    .huart    = &huart3,
    .motor_id = 0
});

void unitree_task_init(void) {
    HAL_Delay(500);
    unitree_motor.enable();
    uart_log.printf("Unitree motor enabled.\r\n");
}


void unitree_task_proc(void) {
    UnitreeMotor::Command cmd = {
        .T   = 0.0f,
        .W   = 0.0f,
        .Pos = 0.0f,
        .K_P = 0.0f,
        .K_W = 0.0f,
    };

    bool ok = unitree_motor.sendRecv(cmd);
    const auto &fb = unitree_motor.getFeedback();

    // 每 100 次打印一次反馈
    static uint32_t cnt = 0;
    if (++cnt % 100 == 0) {
        uart_log.printf("UT: ok=%d valid=%d T=%.2f W=%.2f Pos=%.2f Temp=%d Err=%d\r\n",
            ok, fb.valid, fb.T, fb.W, fb.Pos, fb.Temp, fb.MError);
    }
}
