#include "snail_motor.h"

SnailMotor::SnailMotor(const Config &config)
    : config_(config) {}

void SnailMotor::enable() {
    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_l, config_.pwm_min);
    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_r, config_.pwm_min);

    HAL_TIM_PWM_Start(config_.htim, config_.channel_l);
    HAL_TIM_PWM_Start(config_.htim, config_.channel_r);

    HAL_Delay(2000);
    state_ = State::Enabled;
}

void SnailMotor::disable() {
    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_l, config_.pwm_stop);
    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_r, config_.pwm_stop);
    current_pwm_ = config_.pwm_min;
    state_ = State::Stopped;
}

void SnailMotor::setSpeed(uint16_t speed) {
    if (speed > config_.pwm_max) speed = config_.pwm_max;
    if (speed < config_.pwm_min) speed = config_.pwm_min;

    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_l, speed);
    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_r, speed);
}

void SnailMotor::smoothControl(uint16_t target) {
    uint32_t now = HAL_GetTick();

    if (now - last_time_ >= 5) {
        last_time_ = now;

        if (current_pwm_ != (int16_t)target) {
            if (current_pwm_ < (int16_t)target) current_pwm_ += 2;
            else current_pwm_ -= 2;

            if (current_pwm_ > (int16_t)config_.pwm_max) current_pwm_ = (int16_t)config_.pwm_max;
            if (current_pwm_ < (int16_t)config_.pwm_min) current_pwm_ = (int16_t)config_.pwm_min;

            __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_l, current_pwm_);
            __HAL_TIM_SET_COMPARE(config_.htim, config_.channel_r, current_pwm_);
        }
    }
}
