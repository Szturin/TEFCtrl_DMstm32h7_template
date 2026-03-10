#include "dji_motor.h"

// 静态成员初始化
FDCANInstance DJIMotor::senders_[6] = {};
uint8_t  DJIMotor::sender_enable_[6] = {};
bool     DJIMotor::sender_inited_ = false;
DJIMotor *DJIMotor::instances_[MAX_MOTORS] = {};
uint8_t   DJIMotor::count_ = 0;

void DJIMotor::initSenders() {
    uint16_t std_ids[3] = {0x1FF, 0x200, 0x2FF};
    for (int i = 0; i < 6; i++) {
        senders_[i].fdcan_handle = (i < 3) ? &hfdcan1 : &hfdcan2;
        senders_[i].txconf.Identifier = std_ids[i % 3];
        senders_[i].txconf.IdType = FDCAN_STANDARD_ID;
        senders_[i].txconf.TxFrameType = FDCAN_DATA_FRAME;
        senders_[i].txconf.DataLength = FDCAN_DLC_BYTES_8;
        senders_[i].txconf.BitRateSwitch = FDCAN_BRS_OFF;
        senders_[i].txconf.FDFormat = FDCAN_CLASSIC_CAN;
        senders_[i].txconf.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        senders_[i].txconf.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        senders_[i].txconf.MessageMarker = 0;
        memset(senders_[i].tx_buff, 0, sizeof(senders_[i].tx_buff));
    }
    sender_inited_ = true;
}

DJIMotor::DJIMotor(const Config &config)
    : config_(config)
{
    if (!sender_inited_) initSenders();

    setupSenderGroup();

    // 注册 FDCANInstance 用于接收反馈
    FDCAN_Init_Config_s fdcan_cfg = {};
    fdcan_cfg.fdcan_handle = config_.hfdcan;
    fdcan_cfg.tx_id   = 0;  // 发送走静态 senders_
    fdcan_cfg.data_len = 8;
    fdcan_cfg.fdcan_module_callback = decodeCallback;
    fdcan_cfg.id = this;

    // 计算反馈 rx_id
    uint8_t mid = config_.motor_id - 1;  // 0-based
    if (config_.type == Type::M2006 || config_.type == Type::M3508) {
        fdcan_cfg.rx_id = 0x200 + mid + 1;
    } else {  // GM6020
        fdcan_cfg.rx_id = 0x204 + mid + 1;
    }
    FDCANRegister(&fdcan_cfg);

    if (count_ < MAX_MOTORS) {
        instances_[count_++] = this;
    }
}

void DJIMotor::setupSenderGroup() {
    uint8_t mid = config_.motor_id - 1;  // 0-based
    uint8_t group_idx;

    if (config_.type == Type::M2006 || config_.type == Type::M3508) {
        if (mid < 4) {
            group_idx = (config_.hfdcan == &hfdcan1) ? 1 : 4;  // tx 0x200
            message_num_ = mid;
        } else {
            group_idx = (config_.hfdcan == &hfdcan1) ? 0 : 3;  // tx 0x1FF
            message_num_ = mid - 4;
        }
    } else {  // GM6020
        if (mid < 4) {
            group_idx = (config_.hfdcan == &hfdcan1) ? 0 : 3;  // tx 0x1FF
            message_num_ = mid;
        } else {
            group_idx = (config_.hfdcan == &hfdcan1) ? 2 : 5;  // tx 0x2FF
            message_num_ = mid - 4;
        }
    }

    sender_group_ = group_idx;
    sender_enable_[group_idx] = 1;
}

void DJIMotor::decodeCallback(FDCANInstance *instance) {
    uint8_t *rxbuff = instance->rx_buff;
    DJIMotor *motor = (DJIMotor *)instance->id;
    Feedback *m = &motor->measure_;

    m->last_ecd = m->ecd;
    m->ecd = ((uint16_t)rxbuff[0]) << 8 | rxbuff[1];
    m->angle_single_round = ECD_ANGLE_COEF_DJI * (float)m->ecd;

    m->speed_rpm = (int16_t)((1.0f - SPEED_SMOOTH_COEF) * (float)m->speed_rpm +
                             SPEED_SMOOTH_COEF * (float)((int16_t)(rxbuff[2] << 8 | rxbuff[3])));
    m->real_current = (int16_t)((1.0f - CURRENT_SMOOTH_COEF) * (float)m->real_current +
                                CURRENT_SMOOTH_COEF * (float)((int16_t)(rxbuff[4] << 8 | rxbuff[5])));
    m->temperature = rxbuff[6];

    if (m->ecd - m->last_ecd > 4096) m->total_round--;
    else if (m->ecd - m->last_ecd < -4096) m->total_round++;
    m->total_angle = (float)m->total_round * 360.0f + m->angle_single_round;
}

void DJIMotor::enable() {
    state_ = State::Enabled;
}

void DJIMotor::disable() {
    output_set_ = 0;
    state_ = State::Stopped;
}

void DJIMotor::setCurrent(int16_t current) {
    output_set_ = current;
}

void DJIMotor::sendAll() {
    // 先清空所有发送缓冲
    for (uint8_t i = 0; i < 6; i++) {
        if (sender_enable_[i]) {
            memset(senders_[i].tx_buff, 0, 8);
        }
    }

    // 填充各电机电流值
    for (uint8_t i = 0; i < count_; i++) {
        DJIMotor *motor = instances_[i];
        uint8_t group = motor->sender_group_;
        uint8_t num   = motor->message_num_;
        int16_t set   = (motor->state_ == State::Stopped) ? 0 : motor->output_set_;

        senders_[group].tx_buff[2 * num]     = (uint8_t)(set >> 8);
        senders_[group].tx_buff[2 * num + 1] = (uint8_t)(set & 0xFF);
    }

    // 发送
    for (uint8_t i = 0; i < 6; i++) {
        if (sender_enable_[i]) {
            FDCANTransmit(&senders_[i], 8);
        }
    }
}
