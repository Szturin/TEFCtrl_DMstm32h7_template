#include "motor_task.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include "motor/dm_motor/dm_motor.h"
extern Uart_Instance uart_log;

static float target_speed = 0.0f;  // 目标速度 (rad/s)

DMMotor motor1({
   .motor_id = 0x01,
   .set_mode = MIT_MODE,
   .hcan = &hfdcan1,
   .p_max = 12.5f,
   .v_max = 500.0f,
   .t_max = 10.0f
});

DMMotor motor2({
    .motor_id = 0x02,
    .set_mode = MIT_MODE,
    .hcan = &hfdcan1,
    .p_max = 12.5f,
    .v_max = 500.0f,
    .t_max = 10.0f
});


// 电机任务初始化
void motor_task_init(void) {
    HAL_Delay(800);
    motor1.enable();
    uart_log.printf("Motor enabled, starting control...\r\n\r\n");
}

// 电机任务处理函数 (5ms周期)
void motor_task_proc(void) {

    static float t = 0.0f;
    t += 0.005f;  // 5ms = 0.005s
    target_speed = 500.0f * sinf(t);  // 正弦波速度

    // 更新速度设定值
    motor[Motor1].ctrl.vel_set = target_speed;

    // 发送速度控制命令
    //dm_motor_ctrl_send(&hfdcan1, &motor[Motor1]);
    //spd_ctrl(&hfdcan1, 1, motor[Motor1].ctrl.vel_set);
    motor1.setSpeed(50);
    //motor1.setPosition(target_speed,50.0f);
    motor1.setMIT(0,200,0,0.08,0);

}
