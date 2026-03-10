#pragma once

#include "modules/motor/motor_base.h"
#include "bsp/can/bsp_fdcan.h"
#include <cstdint>
#include <cstring>

class DMMotor : public MotorBase {
public:
    // 控制模式偏移量
    static constexpr uint16_t MIT_MODE = 0x000;
    static constexpr uint16_t POS_MODE = 0x100;
    static constexpr uint16_t SPD_MODE = 0x200;
    static constexpr uint16_t PSI_MODE = 0x300;

    // 寄存器地址
    enum RegisterID : uint8_t {
        RID_UV_VALUE = 0,  RID_KT_VALUE = 1,  RID_OT_VALUE = 2,  RID_OC_VALUE = 3,
        RID_ACC      = 4,  RID_DEC      = 5,  RID_MAX_SPD  = 6,  RID_MST_ID   = 7,
        RID_ESC_ID   = 8,  RID_TIMEOUT  = 9,  RID_CMODE    = 10, RID_DAMP     = 11,
        RID_INERTIA  = 12, RID_HW_VER   = 13, RID_SW_VER   = 14, RID_SN       = 15,
        RID_NPP      = 16, RID_RS       = 17, RID_LS       = 18, RID_FLUX     = 19,
        RID_GR       = 20, RID_PMAX     = 21, RID_VMAX     = 22, RID_TMAX     = 23,
        RID_I_BW     = 24, RID_KP_ASR   = 25, RID_KI_ASR   = 26, RID_KP_APR   = 27,
        RID_KI_APR   = 28, RID_OV_VALUE = 29, RID_GREF     = 30, RID_DETA     = 31,
        RID_V_BW     = 32, RID_IQ_CL    = 33, RID_VL_CL    = 34, RID_CAN_BR   = 35,
        RID_SUB_VER  = 36,
        RID_U_OFF    = 50, RID_V_OFF    = 51, RID_K1       = 52, RID_K2       = 53,
        RID_M_OFF    = 54, RID_DIR      = 55,
        RID_P_M      = 80, RID_X_OUT    = 81
    };

    struct Config {
        uint16_t motor_id;
        uint16_t set_mode;
        hcan_t  *hcan;
        float    p_max;
        float    v_max;
        float    t_max;
        uint16_t master_id = 0x00;  // 反馈 CAN ID（所有 DM 电机共用）
    };

    struct Feedback {
        float position;
        float velocity;
        float torque;
        float temperature_mos;
        float temperature_coil;
        int   error_state;
    };

    explicit DMMotor(const Config &config);

    void enable() override;
    void disable() override;

    // 4 种控制模式
    void setMIT(float pos, float vel, float kp, float kd, float torque);
    void setPosition(float pos, float vel);
    void setSpeed(float vel);
    void setHybrid(float pos, float vel, float current);

    // 辅助命令
    void savePositionZero();
    void clearError();

    // 寄存器读写（通过 0x7FF）
    void readRegister(uint8_t rid);
    void writeRegister(uint8_t rid, const uint8_t data[4]);
    void saveRegister(uint8_t rid);

    const Feedback &getFeedback() const { return feedback_; }
    uint16_t getMotorId() const { return config_.motor_id; }

private:
    Config   config_;
    Feedback feedback_{};

    void sendCANFrame(uint16_t id, const uint8_t *data, uint8_t len);
    void decodeFeedback(const uint8_t *data);

    static float clamp(float x, float min, float max);
    static float uintToFloat(uint16_t x_int, float x_min, float x_max, uint8_t bits);
    static uint16_t floatToUint(float x, float x_min, float x_max, uint8_t bits);

    // 静态实例管理 + CAN 回调
    static constexpr uint8_t MAX_MOTORS = 10;
    static DMMotor  *instances_[MAX_MOTORS];
    static uint8_t   count_;
    static FDCANInstance *shared_can_instance_;
    static void canRxCallback(FDCANInstance *instance);
};
