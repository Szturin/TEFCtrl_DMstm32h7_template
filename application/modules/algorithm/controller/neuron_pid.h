#pragma once

/**
 * @brief 单神经元自适应 PID 控制器
 *
 * 结构: 3 输入感知器，权重 = PID 增益，在线 Hebbian 学习
 *
 *   输入:
 *     x[0] = e(k)                       → P 分量
 *     x[1] = Σe·Ts                      → I 分量（累积）
 *     x[2] = (e(k) - e(k-1)) / Ts       → D 分量
 *
 *   输出:
 *     u = K · Σ(wn[i]·x[i]) + FF·target
 *     wn[i] = w[i] / Σ|w[i]|            （权重归一化）
 *
 *   学习规则 (监督 Hebbian):
 *     Δw[i] = η[i] · e(k) · x[i]
 *     ΔFF   = η_ff · e(k) · target
 *
 * 用法:
 *     NeuronPID pid;
 *     pid.init({...});
 *     float u = pid.update(actual, target, Ts);
 */

#include <cstdint>

struct NeuronPID_Config {
    float K;                // 神经元总增益
    float w_p, w_i, w_d;   // P/I/D 初始权重
    float ff;               // 前馈初始值（0 = 从零学）
    float eta_p, eta_i, eta_d;  // P/I/D 学习率
    float eta_ff;           // 前馈学习率
    float output_max;       // 输出限幅 (±)
    float integral_max;     // 积分限幅 (±)
    float ff_max;           // 前馈权重上限
};

class NeuronPID {
public:
    void init(const NeuronPID_Config &cfg);
    float update(float actual, float target, float Ts);
    void reset();  // 清零内部状态，保留权重

    // 读取学到的参数（VOFA+ 显示用）
    float getW_P()  const { return w_[0]; }
    float getW_I()  const { return w_[1]; }
    float getW_D()  const { return w_[2]; }
    float getFF()   const { return ff_; }
    float getOutput() const { return output_; }

    // 运行时修改学习率（调试用）
    void setEta(float eta_p, float eta_i, float eta_d, float eta_ff);

    // 冻结/解冻学习（收敛后可冻结）
    void freeze()  { frozen_ = true; }
    void unfreeze(){ frozen_ = false; }
    bool isFrozen() const { return frozen_; }

private:
    // 参数
    float K_;
    float w_[3];
    float ff_;
    float eta_[3];
    float eta_ff_;
    float output_max_;
    float integral_max_;
    float ff_max_;

    // 状态
    float integral_;
    float e_prev_;
    float output_;
    bool  frozen_;
};
