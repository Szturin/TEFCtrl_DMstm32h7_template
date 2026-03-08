//
// Created by 123 on 2026/1/17.
//
#include "tim.h"
#include "snail_c615.h"

/* 信号范围定义 (单位: us，假设定时器配置为 1MHz 计数频率) */
#define SNAIL_PWM_MIN     1000     // 安全停转/启动信号 (通常 1000 为标准最低值)
#define SNAIL_PWM_MAX     2000     // 最大动力信号
#define SNAIL_PWM_STOP    400      // 说明书定义的绝对最小值

#define SNAIL_CHANNEL_L   TIM_CHANNEL_1
#define SNAIL_CHANNEL_R   TIM_CHANNEL_3

void snail_init(void) {
    // 1. 初始输出最低有效脉宽，防止上电即旋转
    __HAL_TIM_SET_COMPARE(&htim2, SNAIL_CHANNEL_L, SNAIL_PWM_MIN);
    __HAL_TIM_SET_COMPARE(&htim2, SNAIL_CHANNEL_R, SNAIL_PWM_MIN);

    // 2. 开启 PWM 输出
    HAL_TIM_PWM_Start(&htim2, SNAIL_CHANNEL_L);
    HAL_TIM_PWM_Start(&htim2, SNAIL_CHANNEL_R);

    // 3. 等待电调自检完成 (听到“开机音”表示就绪)
    HAL_Delay(2000);
}

// 建议将此函数暴露给外部使用
void snail_set_speed(uint16_t speed) {
    // 限幅保护，防止超出 C615 允许的范围 [cite: 85, 86]
    if (speed > SNAIL_PWM_MAX) speed = SNAIL_PWM_MAX;
    if (speed < SNAIL_PWM_MIN) speed = SNAIL_PWM_MIN;

    __HAL_TIM_SET_COMPARE(&htim2, SNAIL_CHANNEL_L, speed);
    __HAL_TIM_SET_COMPARE(&htim2, SNAIL_CHANNEL_R, speed);
}

// 优化后的快速停转 (安全起见，停转通常不需要慢速减速，应直接切断动力)
void snail_emergency_stop(void) {
    snail_set_speed(SNAIL_PWM_MIN);
}

void snail_fast_control(uint16_t target) {
    static uint32_t last_time = 0;
    static int16_t current_pwm = 1000; // 初始为 1000 (停止状态)

    uint32_t now = HAL_GetTick();

    // 每 5 毫秒更新一次，频率约 200Hz，符合 C615 的 500Hz 上限
    if (now - last_time >= 5) {
        last_time = now;

        if (current_pwm != target) {
            // 每次改变 2 个单位，手感会非常平滑
            if (current_pwm < target) current_pwm += 2;
            else current_pwm -= 2;

            // 限幅保护，防止超出 C615 允许的 400-2200us 范围 [cite: 85, 86]
            if (current_pwm > 2000) current_pwm = 2000;
            if (current_pwm < 1000) current_pwm = 1000;

            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, current_pwm);
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, current_pwm);
        }
    }
}