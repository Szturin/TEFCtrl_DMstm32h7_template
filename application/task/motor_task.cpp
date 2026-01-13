#include "motor_task.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include "motor/dm_motor/dm_motor.h"

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

DMMotor motor3({
   .motor_id = 0x03,
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
    motor3.enable();
    HAL_Delay(300);
    uart_log.printf("Motor enabled, starting control...\r\n\r\n");
}

// 电机任务处理函数 (5ms周期)
void motor_task_proc(void) {

    static float t = 0.0f;
    t += 0.005f;  // 5ms = 0.005s
    target_speed = 500.0f * sinf(t);  // 正弦波速度

    //位置+速度模式
    motor1.setPosition(0.0,5.0);
    motor2.setPosition(0.0,5.0);
    motor3.setPosition(0.0,5.0);
}
