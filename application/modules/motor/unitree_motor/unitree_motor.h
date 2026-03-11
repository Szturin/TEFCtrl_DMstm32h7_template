-]#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus

#include "usart.h"

class UnitreeMotor {
public:
    enum Type { GO_M8010_6, A1 };

    struct Config {
        Type type;
        UART_HandleTypeDef *huart;
        uint8_t motor_id;           // 0~14
    };

    struct Command {
        float T;      // 前馈力矩 (Nm)
        float W;      // 目标速度 (rad/s)
        float Pos;    // 目标位置 (rad)
        float K_P;    // 位置刚度
        float K_W;    // 速度阻尼
    };

    struct Feedback {
        float T;      // 实际力矩 (Nm)
        float W;      // 实际速度 (rad/s, 关节侧)
        float Pos;    // 实际位置 (rad)
        int   Temp;   // 温度 (deg C)
        uint8_t MError;
        bool  valid;  // CRC 校验通过
    };

    UnitreeMotor(const Config &cfg);

    bool sendRecv(const Command &cmd);
    bool enable();
    bool disable();

    const Feedback& getFeedback() const { return fb_; }

private:
    Config cfg_;
    Feedback fb_;

    // GO-M8010-6 协议
    bool sendRecv_GO(const Command &cmd);
    // A1 协议
    bool sendRecv_A1(const Command &cmd);

    // CRC
    static uint16_t crc_ccitt(uint16_t crc, const uint8_t *buf, size_t len);
    static uint32_t crc32_core(uint32_t *ptr, uint32_t len);
};

#endif // __cplusplus
