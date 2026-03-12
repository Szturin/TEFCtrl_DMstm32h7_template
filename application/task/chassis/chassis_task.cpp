/**
 * @file  chassis_task.cpp
 * @brief 麦克纳姆轮底盘控制
 *
 * 电机布局（俯视）:
 *   LF(1) ---- RF(2)
 *     |   前方   |
 *   LB(3) ---- RB(4)
 *
 * 运动学解算（与老代码一致）:
 *   lf = -vx - vy - L*wz
 *   lb = -vx + vy - L*wz
 *   rb = +vx + vy - L*wz
 *   rf = +vx - vy - L*wz
 */

#include "chassis_task.h"
#include "motor/dji_motor/dji_motor.h"
#include "algorithm/controller/pid.h"
#include "modules/remote/remoter_uart.h"
#include "chassis_power.h"
#include "vofa+/vofa.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ===================== 底盘参数 =====================

// M3508 减速比 19:1, 轮径约 152mm
#define M3508_RATIO           19.0f
#define M3508_RPM_TO_RADS     (2.0f * 3.14159265f / 60.0f / M3508_RATIO)

// 轮距中心距（米），用于 wz 到轮速的转换
// 按 RM 步兵底盘典型值，可根据实际修改
#define CHASSIS_HALF_TRACK    0.18f   // 左右轮距/2
#define CHASSIS_HALF_BASE     0.18f   // 前后轴距/2
#define CHASSISMOTOR_TO_CENTER (CHASSIS_HALF_TRACK + CHASSIS_HALF_BASE)

// 遥控器通道归一化：ch 范围 [-1024, +1023] → [-1, +1]
#define RC_CH_NORMALIZE       (1.0f / 1024.0f)

// 遥控器映射到底盘速度的最大值 (rad/s，减速比后)
#define RC_MAX_VX             8.0f    // 前后最大速度
#define RC_MAX_VY             8.0f    // 左右最大速度
#define RC_MAX_WZ             6.0f    // 旋转最大速度

// 遥控器死区（原始值）
#define RC_DEADZONE           30

// ===================== 电机实例 =====================

static DJIMotor chassis_motor[4] = {
    DJIMotor({.type = DJIMotor::Type::M3508, .hfdcan = &hfdcan1, .motor_id = 1}), // LF
    DJIMotor({.type = DJIMotor::Type::M3508, .hfdcan = &hfdcan1, .motor_id = 2}), // RF
    DJIMotor({.type = DJIMotor::Type::M3508, .hfdcan = &hfdcan1, .motor_id = 3}), // LB
    DJIMotor({.type = DJIMotor::Type::M3508, .hfdcan = &hfdcan1, .motor_id = 4}), // RB
};

// ===================== 速度 PID =====================

static PID_TypeDef chassis_speed_pid[4] = {
    {.Kp = 1050.0f, .Ki = 10.0f, .Kd = 5.0f, .FF_gain = 1.0f, .Output_Max = 10000.0f, .Integral_Max = 3000.0f, .EIS_Max = 10.0f},
    {.Kp = 1050.0f, .Ki = 10.0f, .Kd = 5.0f, .FF_gain = 1.0f, .Output_Max = 10000.0f, .Integral_Max = 3000.0f, .EIS_Max = 10.0f},
    {.Kp = 1050.0f, .Ki = 10.0f, .Kd = 5.0f, .FF_gain = 1.0f, .Output_Max = 10000.0f, .Integral_Max = 3000.0f, .EIS_Max = 10.0f},
    {.Kp = 1050.0f, .Ki = 10.0f, .Kd = 5.0f, .FF_gain = 1.0f, .Output_Max = 10000.0f, .Integral_Max = 3000.0f, .EIS_Max = 10.0f},
};

// ===================== 状态变量 =====================

static Chassis_Mode_e chassis_mode = CHASSIS_FREE;
static float chassis_vx = 0;  // 底盘前后速度 (rad/s)
static float chassis_vy = 0;  // 底盘左右速度 (rad/s)
static float chassis_wz = 0;  // 底盘旋转速度 (rad/s)
static float wheel_speed_set[4] = {0};  // 四轮目标速度

// 小陀螺速度比例 (0~1, 由旋钮 LD 控制)
static float rotate_ratio = 0.5f;

// ===================== 辅助函数 =====================

/** 遥控器死区处理 */
static inline float rc_deadzone(int16_t ch) {
    if (ch > RC_DEADZONE) return (float)(ch - RC_DEADZONE);
    if (ch < -RC_DEADZONE) return (float)(ch + RC_DEADZONE);
    return 0.0f;
}

// ===================== 初始化 =====================

void chassis_task_init(void) {
    for (int i = 0; i < 4; i++) {
        PID_Reset(&chassis_speed_pid[i]);
        chassis_motor[i].enable();
    }
    // 初始化功率控制模块（含功率计）
    chassis_power_init(&hfdcan1);
}

// ===================== 遥控器输入处理 =====================

static void chassis_rc_input(void) {
    // 通道映射 (ET16S WBUS):
    //   ch1 = 左摇杆 X → vy (左右平移)
    //   ch2 = 左摇杆 Y → vx (前后)
    //   ch4 = 右摇杆 X → wz (旋转)
    //   SA  = 模式拨杆

    float vx_raw = rc_deadzone(wbus_rc.remote.ch3) * RC_CH_NORMALIZE * 5.0f;
    float vy_raw = rc_deadzone(wbus_rc.remote.ch4) * RC_CH_NORMALIZE *5.0f;
    float wz_raw = rc_deadzone(wbus_rc.remote.ch1) * RC_CH_NORMALIZE * 15.0f;

    chassis_vx = vx_raw * RC_MAX_VX;
    chassis_vy = vy_raw * RC_MAX_VY;

    // 模式切换 (SA: -1=零力, 0=自由, +1=小陀螺)
    if (wbus_rc.remote.SA < 0) {
        chassis_mode = CHASSIS_ZERO_FORCE;
    } else if (wbus_rc.remote.SA == 0) {
        chassis_mode = CHASSIS_FREE;
    } else {
        chassis_mode = CHASSIS_ROTATE;
    }

    // 小陀螺速度由旋钮 LD 控制
    rotate_ratio = ((float)wbus_rc.remote.LD + 1024.0f) / 2048.0f;  // 归一化 0~1

    switch (chassis_mode) {
    case CHASSIS_ZERO_FORCE:
        chassis_vx = 0;
        chassis_vy = 0;
        chassis_wz = 0;
        break;
    case CHASSIS_FREE:
        chassis_wz = wz_raw * RC_MAX_WZ;
        break;
    case CHASSIS_FOLLOW_GIMBAL_YAW:
        // 需要云台模块提供偏角，暂时同 FREE
        chassis_wz = wz_raw * RC_MAX_WZ;
        break;
    case CHASSIS_ROTATE:
        chassis_wz = -RC_MAX_WZ * rotate_ratio;
        // 小陀螺模式下仍允许遥控器控制平移
        break;
    }
}

// ===================== 麦克纳姆轮运动学解算 =====================

static void chassis_mecanum_calc(void) {
    // 与老代码一致的解算方向
    wheel_speed_set[0] = -chassis_vx - chassis_vy - CHASSISMOTOR_TO_CENTER * chassis_wz; // LF
    wheel_speed_set[1] = +chassis_vx - chassis_vy - CHASSISMOTOR_TO_CENTER * chassis_wz; // RF
    wheel_speed_set[2] = -chassis_vx + chassis_vy - CHASSISMOTOR_TO_CENTER * chassis_wz; // LB
    wheel_speed_set[3] = +chassis_vx + chassis_vy - CHASSISMOTOR_TO_CENTER * chassis_wz; // RB
}

// ===================== PID 速度闭环 =====================

static void chassis_motor_ctrl(void) {
    int16_t current[4] = {0};

    // 1. PID 计算各轮电流
    for (int i = 0; i < 4; i++) {
        if (chassis_mode == CHASSIS_ZERO_FORCE) {
            chassis_motor[i].setCurrent(0);
            PID_Reset(&chassis_speed_pid[i]);
            continue;
        }

        float actual = chassis_motor[i].getFeedback().speed_rpm * M3508_RPM_TO_RADS;
        float output = Position_PID(&chassis_speed_pid[i], actual, wheel_speed_set[i]);

        if (output >  10000.0f) output =  10000.0f;
        if (output < -10000.0f) output = -10000.0f;

        current[i] = (int16_t)output;
    }

    if (chassis_mode == CHASSIS_ZERO_FORCE) return;

    // 2. 功率限制（模型前馈 + 大P分配）
    float speed_actual[4];
    for (int i = 0; i < 4; i++)
        speed_actual[i] = chassis_motor[i].getFeedback().speed_rpm * M3508_RPM_TO_RADS;
    chassis_power_limit(current, wheel_speed_set, speed_actual);

    // 3. 下发电流
    for (int i = 0; i < 4; i++) {
        chassis_motor[i].setCurrent(current[i]);
    }
}

// ===================== 主循环 (1ms) =====================

void chassis_task_proc(void) {
    chassis_rc_input();
    chassis_mecanum_calc();
    chassis_motor_ctrl();

    // VOFA+ 降频输出（每 10ms 一次）
    static uint32_t vofa_cnt = 0;
    if (++vofa_cnt % 10 == 0) {
        float vofa_data[] = {
            chassis_power_get_estimated(),  // CH0: 模型估算功率 (W)
            chassis_power_get_measured(),   // CH1: 功率计实测功率 (W)
            chassis_power_get_limit(),      // CH2: 功率上限参考线 (W)
        };
        vofa_justfloat_send(vofa_data, sizeof(vofa_data) / sizeof(vofa_data[0]));
    }
}

// ===================== USB 命令解析（调试） =====================
// stop           — 零力模式
// free           — 自由模式
// rotate         — 小陀螺
// pid:Kp,Ki,Kd   — 设置速度 PID
// eis:10         — 积分分离阈值
// plimit:80      — 设置功率上限 (W)
// plimit:off     — 关闭功率限制
// plimit:on      — 开启功率限制

void chassis_cmd_parse(const char *cmd) {
    if (strcmp(cmd, "stop") == 0) {
        chassis_mode = CHASSIS_ZERO_FORCE;
        for (int i = 0; i < 4; i++) PID_Reset(&chassis_speed_pid[i]);

    } else if (strcmp(cmd, "free") == 0) {
        chassis_mode = CHASSIS_FREE;

    } else if (strcmp(cmd, "rotate") == 0) {
        chassis_mode = CHASSIS_ROTATE;

    } else if (strncmp(cmd, "pid:", 4) == 0) {
        char buf[32];
        strncpy(buf, cmd + 4, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        float vals[3] = {
            chassis_speed_pid[0].Kp, chassis_speed_pid[0].Ki, chassis_speed_pid[0].Kd
        };
        char *tok = strtok(buf, ",");
        for (int i = 0; i < 3 && tok; i++, tok = strtok(nullptr, ","))
            vals[i] = (float)atof(tok);
        for (int i = 0; i < 4; i++) {
            chassis_speed_pid[i].Kp = vals[0];
            chassis_speed_pid[i].Ki = vals[1];
            chassis_speed_pid[i].Kd = vals[2];
            PID_Reset(&chassis_speed_pid[i]);
        }

    } else if (strncmp(cmd, "eis:", 4) == 0) {
        float val = (float)atof(cmd + 4);
        for (int i = 0; i < 4; i++)
            chassis_speed_pid[i].EIS_Max = val;

    } else {
        chassis_power_cmd_parse(cmd);
    }
}
