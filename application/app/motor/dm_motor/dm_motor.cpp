//
// Created by 123 on 2026/1/10.
//
#include "dm_motor.h"

// 控制模式偏移量


#define KP_MIN 0.0f
#define KP_MAX 500.0f
#define KD_MIN 0.0f
#define KD_MAX 5.0f

// 构造函数
DMMotor::DMMotor(const Config& config)
    : config_(config),  //拷贝config
      feedback_{}       //零初始化feedback
{
    // 参数校验
    if(config_.hcan == nullptr){
        // 错误处理
        return;
    }

    if(config_.p_max <= 0.0f || config_.v_max <= 0.0f ||
        config_.t_max <= 0.0f){
        // 错误处理
        return;
    }
}

// 协议要阅读手册！
void DMMotor::enable(){
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    uint16_t can_id = config_.motor_id + config_.set_mode; // 使能电机模式
    sendCANFrame(can_id, data, 8);

}

void DMMotor::disable(){
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
    uint16_t can_id = config_.motor_id + config_.set_mode; // 失能电机模式
    sendCANFrame(can_id, data, 8);
}
void DMMotor::sendCANFrame(uint16_t id, const uint8_t *data, 
                           uint8_t len){
    // BSP层的发送函数
    fdcanx_send_data(config_.hcan, id,
                     const_cast<uint8_t*>(data),len);
}

void DMMotor::setPosition(float pos, float vel){

    uint8_t data[8];

    // 直接内存拷贝
    memcpy(&data[0], &pos, sizeof(float));
    memcpy(&data[4], &vel, sizeof(float));

    uint16_t can_id = 0x101; // POS_MODE = 0X100
    sendCANFrame(can_id,data,8);
}

// ========== 速度模式 ==========

void DMMotor::setSpeed(float vel) {
    vel = clamp(vel, -config_.v_max, config_.v_max);

    uint8_t data[4];
    memcpy(data, &vel, sizeof(float));

    uint16_t can_id = config_.motor_id + SPD_MODE;
    sendCANFrame(can_id, data, 4);
}

// ========== MIT模式 ==========

void DMMotor::setMIT(float pos, float vel, float kp, float kd, float torque) {
    // 参数裁剪
    pos = clamp(pos, -config_.p_max, config_.p_max);
    vel = clamp(vel, -config_.v_max, config_.v_max);
    torque = clamp(torque, -config_.t_max, config_.t_max);
    kp = clamp(kp, KP_MIN, KP_MAX);
    kd = clamp(kd, KD_MIN, KD_MAX);

    // 浮点数转整数
    uint16_t pos_tmp = floatToUint(pos, -config_.p_max, config_.p_max, 16);
    uint16_t vel_tmp = floatToUint(vel, -config_.v_max, config_.v_max, 12);
    uint16_t kp_tmp  = floatToUint(kp, KP_MIN, KP_MAX, 12);
    uint16_t kd_tmp  = floatToUint(kd, KD_MIN, KD_MAX, 12);
    uint16_t tor_tmp = floatToUint(torque, -config_.t_max, config_.t_max, 12);

    // 位打包
    uint8_t data[8];
    data[0] = (pos_tmp >> 8);
    data[1] = pos_tmp & 0xFF;
    data[2] = (vel_tmp >> 4);
    data[3] = ((vel_tmp & 0xF) << 4) | (kp_tmp >> 8);
    data[4] = kp_tmp & 0xFF;
    data[5] = (kd_tmp >> 4);
    data[6] = ((kd_tmp & 0xF) << 4) | (tor_tmp >> 8);
    data[7] = tor_tmp & 0xFF;

    // 发送CAN帧
    uint16_t can_id = config_.motor_id + MIT_MODE;
    sendCANFrame(can_id, data, 8);
}


// ========== 私有辅助函数 ==========

uint16_t DMMotor::floatToUint(float x, float x_min, float x_max, uint8_t bits) {
    // 边界检查
    if (x_max <= x_min) return 0;
    if (bits > 16) bits = 16;

    // 裁剪
    if (x < x_min) x = x_min;
    if (x > x_max) x = x_max;

    // 线性映射
    float span = x_max - x_min;
    float ratio = (x - x_min) / span;
    uint16_t max_val = (1 << bits) - 1;
    return (uint16_t)(ratio * max_val);
}

float DMMotor::uintToFloat(uint16_t x_int, float x_min, float x_max, uint8_t bits) {
    // 边界检查
    if (x_max <= x_min) return 0.0f;
    if (bits > 16) bits = 16;

    // 归一化
    uint16_t max_val = (1 << bits) - 1;
    float ratio = (float)x_int / (float)max_val;

    // 映射到实际范围
    float span = x_max - x_min;
    return ratio * span + x_min;
}

