#include "motor_task.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include "motor/dm_motor/dm_motor.h"
#include "bsp/can/bsp_fdcan.h"
#include "motor/dji_motor/dji_motor.h"
#include "motor/dji_motor/snail_c615.h"

extern Uart_Instance uart_log;

// DM 电机实例（工程机器人两轴）
DMMotor motor1({
    .motor_id = 0x01,
    .set_mode = POS_MODE,
    .hcan     = &hfdcan1,
    .p_max    = 12.5f,
    .v_max    = 500.0f,
    .t_max    = 10.0f
});

DMMotor motor2({
    .motor_id = 0x02,
    .set_mode = POS_MODE,
    .hcan     = &hfdcan1,
    .p_max    = 12.5f,
    .v_max    = 500.0f,
    .t_max    = 10.0f
});

// 电机任务初始化
void motor_task_init(void) {
    HAL_Delay(800);
    motor1.enable();
    HAL_Delay(300);
    motor2.enable();
    HAL_Delay(300);
    uart_log.printf("Motor enabled.\r\n");
}

// 电机任务处理函数 (5ms周期)
void motor_task_proc(void) {
    motor1.setPosition((float)wbus_rc.remote.ch1 * 0.0015f, 8.0);
    motor2.setPosition((float)wbus_rc.remote.ch2 * 0.0015f, 8.0);
}

// ==============================================================================
// 发射机构（M2006 拨蛋盘 + C615 蜗牛电机）
// ==============================================================================
static DJIMotorInstance *shoot_motor;

void shoot_task_init(void) {
    Motor_Init_Config_s shoot_config = {
        .motor_type = M2006,
        .can_init_config = {
            .fdcan_handle = &hfdcan1,
            .tx_id = 1,
        }
    };
    shoot_motor = DJIMotorInit(&shoot_config);
    snail_init();
}

void shoot_task_proc(void) {
    if (wbus_rc.remote.SB != 0) {
        snail_fast_control(1200);
        DJIMotorSetRef_Current(shoot_motor, 1000);
    } else {
        DJIMotorSetRef_Current(shoot_motor, 0);
        snail_fast_control(1000);
    }
    DJIMotorControl();
}
