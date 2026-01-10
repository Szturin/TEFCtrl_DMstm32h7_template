//
// Created by 123 on 2026/1/10.
//

#ifndef DM_MOTOR_H
#define DM_MOTOR_H

#include "bsp_system.h"
#include "main.h"
#include "memorymap.h"
#include <cstdarg>
#include <cstring>
#include <cstdio>

#ifdef __cplusplus
extern "C" { // 存放C接口
#endif

#ifdef __cplusplus
}
#endif


class DMMotor{
public:
    struct Config{
        uint16_t motor_id;  // 电机CAN ID
        uint16_t set_mode;  // 电机控制模式
        hcan_t* hcan;       // CAN总线句柄指针
        float p_max;        // 最大位置（rad）
        float v_max;        // 最大速度（rad/s）
        float t_max;        // 最大扭矩（N*m）
    }; // 电机初始化配置

    struct Feedback{
        float position;         // 当前位置（rad）
        float velocity;         // 当前速度（rad/s）
        float torque;           // 当前扭矩（N*m）
        float temperature_mos;  // MOS管温度 （℃）
        float temperature_coil; // 线圈温度（℃）
        int error_state;        // 错误状态码（查阅手册）
    }; // 电机反馈信息（实时）

    // 构造函数
    explicit DMMotor(const Config& config);

    // 基础控制
    void enable();
    void disable();

    // 4种控制模式
    //1.MIT 2.位置模式 3.速度模式 4.力位混控模式
    void setMIT(float pos, float vel,       // 位置 速度
                float kp, float kd,          // p,d参数
                float torque);              // 输出力矩
    void setPosition(float pos, float vel); // 位置 速度
    void setSpeed(float vel);               // 速度
    void setHybrid(float pos, float vel, float current);// 位置 速度 电流

    //反馈接口
    void onCANReceive(const uint8_t* data, uint8_t len); // 数据报文 长度
    const Feedback& getFeedback() const;

private:
    Config config_; // 配置参数
    Feedback feedback_; // 反馈数据
    /**
     * @brief 发送CAN帧
     * @param id CAN帧ID
     * @param data 数据
     * @param len 长度
     */
    void sendCANFrame(uint16_t id, const uint8_t* data, uint8_t len);
    inline float clamp(float x, float min, float max) {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    float uintToFloat(uint16_t x_int, float x_min, float x_max, uint8_t bits);
    uint16_t floatToUint(float x, float x_min, float x_max, uint8_t bits);

};

#endif //TEFCTRL_DMSTM32H7_TEMPLATE_DM_MOTOR_H
