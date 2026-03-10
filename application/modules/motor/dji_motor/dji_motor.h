#pragma once

#include "modules/motor/motor_base.h"
#include "bsp/can/bsp_fdcan.h"
#include <cstdint>
#include <cstring>

#define ECD_ANGLE_COEF_DJI    0.043945f   // 360/8192
#define SPEED_SMOOTH_COEF     0.95f
#define CURRENT_SMOOTH_COEF   0.9f

class DJIMotor : public MotorBase {
public:
    enum class Type : uint8_t { M2006, M3508, GM6020 };

    struct Config {
        Type type;
        FDCAN_HandleTypeDef *hfdcan;
        uint8_t motor_id;   // 1~8
    };

    struct Feedback {
        uint16_t last_ecd;
        uint16_t ecd;
        float    angle_single_round;
        int16_t  speed_rpm;
        int16_t  real_current;
        uint8_t  temperature;
        float    total_angle;
        int32_t  total_round;
    };

    explicit DJIMotor(const Config &config);

    void enable() override;
    void disable() override;
    void setCurrent(int16_t current);
    const Feedback &getFeedback() const { return measure_; }

    static void sendAll();

    // DEBUG: 获取指定 sender group 的 tx_buff 指针
    static const uint8_t *dbgSenderBuf(uint8_t group) { return senders_[group].tx_buff; }
    static uint32_t dbgSenderTxId(uint8_t group) { return senders_[group].txconf.Identifier; }
    static uint32_t dbgSenderTxDLC(uint8_t group) { return senders_[group].txconf.DataLength; }
    uint8_t dbgGroup() const { return sender_group_; }
    uint8_t dbgMsgNum() const { return message_num_; }

private:
    Config   config_;
    Feedback measure_{};
    int16_t  output_set_ = 0;
    uint8_t  sender_group_ = 0;
    uint8_t  message_num_  = 0;

    void setupSenderGroup();

    // 静态发送器（2 bus * 3 group = 6）
    static FDCANInstance senders_[6];
    static uint8_t sender_enable_[6];
    static bool sender_inited_;

    static constexpr uint8_t MAX_MOTORS = 12;
    static DJIMotor *instances_[MAX_MOTORS];
    static uint8_t   count_;

    static void initSenders();
    static void decodeCallback(FDCANInstance *instance);
};
