#pragma once

#include "modules/motor/motor_base.h"
#include "main.h"

class SnailMotor : public MotorBase {
public:
    struct Config {
        TIM_HandleTypeDef *htim;
        uint32_t channel_l;
        uint32_t channel_r;
        uint16_t pwm_min  = 1000;
        uint16_t pwm_max  = 2000;
        uint16_t pwm_stop = 400;
    };

    explicit SnailMotor(const Config &config);

    void enable() override;
    void disable() override;
    void setSpeed(uint16_t speed);
    void smoothControl(uint16_t target);

private:
    Config config_;
    int16_t current_pwm_ = 1000;
    uint32_t last_time_ = 0;
};
