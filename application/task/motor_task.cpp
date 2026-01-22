#include "motor_task.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include "motor/dm_motor/dm_motor.h"
#include "bsp/can/bsp_fdcan.h"
#include "motor/dji_motor/dji_motor.h"
#include "motor/dji_motor/snail_c615.h"
extern Uart_Instance uart_log;

static float target_speed = 0.0f;  // 目标速度 (rad/s)

DMMotor motor1({
   .motor_id = 0x01,
   .set_mode = POS_MODE,
   .hcan = &hfdcan1,
   .p_max = 12.5f,
   .v_max = 500.0f,
   .t_max = 10.0f
});

DMMotor motor2({
    .motor_id = 0x02,
    .set_mode = POS_MODE,
    .hcan = &hfdcan1,
    .p_max = 12.5f,
    .v_max = 500.0f,
    .t_max = 10.0f
});

// 电机任务初始化
void motor_task_init(void) {
    HAL_Delay(800);
    motor1.enable();
    HAL_Delay(300);
    motor2.enable();
    HAL_Delay(300);
    //motor3.enable();
    //HAL_Delay(300);
    uart_log.printf("Motor enabled, starting control...\r\n\r\n");
}

// 电机任务处理函数 (5ms周期)
void motor_task_proc(void) {

    static float t = 0.0f;
    t += 0.005f;  // 5ms = 0.005s
    target_speed = 500.0f * sinf(t);  // 正弦波速度

    //位置+速度模式
    motor1.setPosition((float)wbus_rc.remote.ch1 * 0.0015f,8.0);
    motor2.setPosition((float)wbus_rc.remote.ch2 * 0.0015f,8.0);
}

// 定义电机实例指针
static DJIMotorInstance *shoot_motor; // 拨蛋盘电机

/*无人机发射机构*/
void shoot_task_init(void) {
    // 配置电机初始化参数
    Motor_Init_Config_s shoot_config = {
            .motor_type = M2006,                // 对应 DJI M2006 电机
            .can_init_config = {
                    .fdcan_handle = &hfdcan1,       // 挂载在 FDCAN1 总线
                    .tx_id = 1,                     // 电调 ID 设置为 1
            }
    };

    // 创建电机实例
    shoot_motor = DJIMotorInit(&shoot_config);
    snail_init();
}

/**
 * @brief 电机任务处理函数 (5ms周期)
 * 包含反馈读取、闭环计算（需根据业务补充）以及指令统一发送
 */
void shoot_task_proc(void) {
    // 获取电机反馈数据（SI 标准单位或原始数据）
    float current_angle = shoot_motor->measure.total_angle; // 累积角度
    int16_t current_speed = shoot_motor->measure.speed_rpm; // 实时转速

    int16_t target_current = 1000; // 假设目标电流为 1000mA

    if(wbus_rc.remote.SB != 0){ // SB按键 使能2006 snail
        snail_fast_control(1200);
        DJIMotorSetRef_Current(shoot_motor, target_current);
    }else{
        DJIMotorSetRef_Current(shoot_motor, 0);
        snail_fast_control(1000);
    }

    // 统一发送所有 DJI 电机的控制报文
    // 注意：此函数会遍历所有已注册电机并分组打包发送，建议在所有电机任务处理完后调用一次
    DJIMotorControl();
}