#include "neuron_pid.h"
#include <cmath>

void NeuronPID::init(const NeuronPID_Config &cfg) {
    K_ = cfg.K;
    w_[0] = cfg.w_p;  w_[1] = cfg.w_i;  w_[2] = cfg.w_d;
    ff_ = cfg.ff;
    eta_[0] = cfg.eta_p;  eta_[1] = cfg.eta_i;  eta_[2] = cfg.eta_d;
    eta_ff_ = cfg.eta_ff;
    output_max_ = cfg.output_max;
    integral_max_ = cfg.integral_max;
    ff_max_ = cfg.ff_max;
    integral_ = 0;
    e_prev_ = 0;
    output_ = 0;
    frozen_ = false;
}

void NeuronPID::reset() {
    integral_ = 0;
    e_prev_ = 0;
    output_ = 0;
}

void NeuronPID::setEta(float eta_p, float eta_i, float eta_d, float eta_ff) {
    eta_[0] = eta_p;  eta_[1] = eta_i;  eta_[2] = eta_d;
    eta_ff_ = eta_ff;
}

float NeuronPID::update(float actual, float target, float Ts) {
    float e = target - actual;

    // ---- 神经元输入 ----
    float x[3];
    x[0] = e;                            // P
    integral_ += e * Ts;                  // I
    if (integral_ >  integral_max_) integral_ =  integral_max_;
    if (integral_ < -integral_max_) integral_ = -integral_max_;
    x[1] = integral_;
    x[2] = (e - e_prev_) / Ts;           // D

    // ---- 权重归一化 ----
    float w_abs = fabsf(w_[0]) + fabsf(w_[1]) + fabsf(w_[2]);
    if (w_abs < 1e-6f) w_abs = 1e-6f;

    // ---- 神经元输出 ----
    float u_pid = 0;
    for (int i = 0; i < 3; i++)
        u_pid += (w_[i] / w_abs) * x[i];
    u_pid *= K_;

    output_ = u_pid + ff_ * target;

    // 限幅
    if (output_ >  output_max_) output_ =  output_max_;
    if (output_ < -output_max_) output_ = -output_max_;

    // ---- 在线学习 ----
    if (!frozen_) {
        for (int i = 0; i < 3; i++) {
            w_[i] += eta_[i] * e * x[i];
            if (w_[i] < 0) w_[i] = 0;     // 非负约束
        }
        ff_ += eta_ff_ * e * target;
        if (ff_ < 0) ff_ = 0;
        if (ff_ > ff_max_) ff_ = ff_max_;
    }

    e_prev_ = e;
    return output_;
}
