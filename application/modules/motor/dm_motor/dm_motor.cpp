#include "dm_motor.h"

static constexpr float KP_MIN = 0.0f;
static constexpr float KP_MAX = 500.0f;
static constexpr float KD_MIN = 0.0f;
static constexpr float KD_MAX = 5.0f;

// 静态成员初始化
DMMotor *DMMotor::instances_[MAX_MOTORS] = {};
uint8_t  DMMotor::count_ = 0;
FDCANInstance *DMMotor::shared_can_instance_ = nullptr;

DMMotor::DMMotor(const Config &config)
    : config_(config)
{
    if (config_.hcan == nullptr) return;
    if (config_.p_max <= 0.0f || config_.v_max <= 0.0f || config_.t_max <= 0.0f) return;

    // 注册到静态实例数组
    if (count_ < MAX_MOTORS) {
        instances_[count_++] = this;
    }

    // 第一个 DMMotor 实例注册 FDCANInstance，后续共享
    if (shared_can_instance_ == nullptr) {
        FDCAN_Init_Config_s fdcan_cfg = {};
        fdcan_cfg.fdcan_handle = config_.hcan;
        fdcan_cfg.tx_id   = 0;  // DM 电机发送不走 FDCANInstance
        fdcan_cfg.rx_id   = config_.master_id;
        fdcan_cfg.data_len = 8;
        fdcan_cfg.fdcan_module_callback = canRxCallback;
        fdcan_cfg.id = nullptr;
        shared_can_instance_ = FDCANRegister(&fdcan_cfg);
    }
}

// ========== CAN 接收回调 ==========

void DMMotor::canRxCallback(FDCANInstance *instance) {
    uint8_t *data = instance->rx_buff;
    uint8_t motor_id_field = data[0] & 0x0F;

    // 寄存器回复（data[2]==0x33）也走这里
    for (uint8_t i = 0; i < count_; i++) {
        if ((instances_[i]->config_.motor_id & 0x0F) == motor_id_field) {
            instances_[i]->decodeFeedback(data);
            return;
        }
    }
}

void DMMotor::decodeFeedback(const uint8_t *data) {
    // data[0] 高4位 = state
    feedback_.error_state = (data[0] >> 4);

    uint16_t p_int = ((uint16_t)data[1] << 8) | data[2];
    uint16_t v_int = ((uint16_t)data[3] << 4) | (data[4] >> 4);
    uint16_t t_int = ((uint16_t)(data[4] & 0x0F) << 8) | data[5];

    feedback_.position = uintToFloat(p_int, -config_.p_max, config_.p_max, 16);
    feedback_.velocity = uintToFloat(v_int, -config_.v_max, config_.v_max, 12);
    feedback_.torque   = uintToFloat(t_int, -config_.t_max, config_.t_max, 12);
    feedback_.temperature_mos  = (float)data[6];
    feedback_.temperature_coil = (float)data[7];

    if (feedback_.error_state != 0) {
        state_ = State::Error;
    }
}

// ========== 基础控制 ==========

void DMMotor::enable() {
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    uint16_t can_id = config_.motor_id + config_.set_mode;
    sendCANFrame(can_id, data, 8);
    state_ = State::Enabled;
}

void DMMotor::disable() {
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
    uint16_t can_id = config_.motor_id + config_.set_mode;
    sendCANFrame(can_id, data, 8);
    state_ = State::Stopped;
}

// ========== 4 种控制模式 ==========

void DMMotor::setMIT(float pos, float vel, float kp, float kd, float torque) {
    pos    = clamp(pos,    -config_.p_max, config_.p_max);
    vel    = clamp(vel,    -config_.v_max, config_.v_max);
    torque = clamp(torque, -config_.t_max, config_.t_max);
    kp     = clamp(kp,     KP_MIN, KP_MAX);
    kd     = clamp(kd,     KD_MIN, KD_MAX);

    uint16_t pos_tmp = floatToUint(pos,    -config_.p_max, config_.p_max, 16);
    uint16_t vel_tmp = floatToUint(vel,    -config_.v_max, config_.v_max, 12);
    uint16_t kp_tmp  = floatToUint(kp,     KP_MIN, KP_MAX, 12);
    uint16_t kd_tmp  = floatToUint(kd,     KD_MIN, KD_MAX, 12);
    uint16_t tor_tmp = floatToUint(torque, -config_.t_max, config_.t_max, 12);

    uint8_t data[8];
    data[0] = (pos_tmp >> 8);
    data[1] = pos_tmp & 0xFF;
    data[2] = (vel_tmp >> 4);
    data[3] = ((vel_tmp & 0xF) << 4) | (kp_tmp >> 8);
    data[4] = kp_tmp & 0xFF;
    data[5] = (kd_tmp >> 4);
    data[6] = ((kd_tmp & 0xF) << 4) | (tor_tmp >> 8);
    data[7] = tor_tmp & 0xFF;

    sendCANFrame(config_.motor_id + MIT_MODE, data, 8);
}

void DMMotor::setPosition(float pos, float vel) {
    uint8_t data[8];
    memcpy(&data[0], &pos, sizeof(float));
    memcpy(&data[4], &vel, sizeof(float));
    sendCANFrame(config_.motor_id + POS_MODE, data, 8);
}

void DMMotor::setSpeed(float vel) {
    vel = clamp(vel, -config_.v_max, config_.v_max);
    uint8_t data[4];
    memcpy(data, &vel, sizeof(float));
    sendCANFrame(config_.motor_id + SPD_MODE, data, 4);
}

void DMMotor::setHybrid(float pos, float vel, float current) {
    uint16_t u16_vel = (uint16_t)(vel * 100);
    uint16_t u16_cur = (uint16_t)(current * 10000);

    uint8_t data[8];
    memcpy(&data[0], &pos, sizeof(float));
    memcpy(&data[4], &u16_vel, sizeof(uint16_t));
    memcpy(&data[6], &u16_cur, sizeof(uint16_t));

    sendCANFrame(config_.motor_id + PSI_MODE, data, 8);
}

// ========== 辅助命令 ==========

void DMMotor::savePositionZero() {
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
    sendCANFrame(config_.motor_id + config_.set_mode, data, 8);
}

void DMMotor::clearError() {
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB};
    sendCANFrame(config_.motor_id + config_.set_mode, data, 8);
}

// ========== 寄存器读写（通过 0x7FF） ==========

void DMMotor::readRegister(uint8_t rid) {
    uint8_t can_id_l = config_.motor_id & 0xFF;
    uint8_t can_id_h = (config_.motor_id >> 8) & 0x07;
    uint8_t data[4] = {can_id_l, can_id_h, 0x33, rid};
    fdcanx_send_data(config_.hcan, 0x7FF, data, 4);
}

void DMMotor::writeRegister(uint8_t rid, const uint8_t data_in[4]) {
    uint8_t can_id_l = config_.motor_id & 0xFF;
    uint8_t can_id_h = (config_.motor_id >> 8) & 0x07;
    uint8_t data[8] = {can_id_l, can_id_h, 0x55, rid,
                       data_in[0], data_in[1], data_in[2], data_in[3]};
    fdcanx_send_data(config_.hcan, 0x7FF, data, 8);
}

void DMMotor::saveRegister(uint8_t rid) {
    uint8_t can_id_l = config_.motor_id & 0xFF;
    uint8_t can_id_h = (config_.motor_id >> 8) & 0x07;
    uint8_t data[4] = {can_id_l, can_id_h, 0xAA, 0x01};
    fdcanx_send_data(config_.hcan, 0x7FF, data, 4);
}

// ========== 发送 ==========

void DMMotor::sendCANFrame(uint16_t id, const uint8_t *data, uint8_t len) {
    fdcanx_send_data(config_.hcan, id, const_cast<uint8_t *>(data), len);
}

// ========== 辅助函数 ==========

float DMMotor::clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

uint16_t DMMotor::floatToUint(float x, float x_min, float x_max, uint8_t bits) {
    if (x_max <= x_min) return 0;
    if (bits > 16) bits = 16;
    if (x < x_min) x = x_min;
    if (x > x_max) x = x_max;
    float span = x_max - x_min;
    uint16_t max_val = (1 << bits) - 1;
    return (uint16_t)((x - x_min) / span * max_val);
}

float DMMotor::uintToFloat(uint16_t x_int, float x_min, float x_max, uint8_t bits) {
    if (x_max <= x_min) return 0.0f;
    if (bits > 16) bits = 16;
    uint16_t max_val = (1 << bits) - 1;
    float span = x_max - x_min;
    return (float)x_int / (float)max_val * span + x_min;
}
