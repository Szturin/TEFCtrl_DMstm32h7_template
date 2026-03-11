#include "unitree_motor.h"
#include <cstring>

// ============================================================================
// CRC-CCITT 查表 (x^0 + x^5 + x^12 + x^16, from Linux kernel)
// ============================================================================
static const uint16_t crc_ccitt_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t UnitreeMotor::crc_ccitt(uint16_t crc, const uint8_t *buf, size_t len)
{
    while (len--)
        crc = (crc >> 8) ^ crc_ccitt_table[(crc ^ *buf++) & 0xff];
    return crc;
}

// ============================================================================
// CRC32 (多项式 0x04C11DB7, 初始值 0xFFFFFFFF, 逐位计算)
// ============================================================================
uint32_t UnitreeMotor::crc32_core(uint32_t *ptr, uint32_t len)
{
    uint32_t xbit, data;
    uint32_t CRC32 = 0xFFFFFFFF;
    const uint32_t dwPolynomial = 0x04c11db7;

    for (uint32_t i = 0; i < len; i++) {
        xbit = 1 << 31;
        data = ptr[i];
        for (uint32_t bits = 0; bits < 32; bits++) {
            if (CRC32 & 0x80000000) {
                CRC32 <<= 1;
                CRC32 ^= dwPolynomial;
            } else {
                CRC32 <<= 1;
            }
            if (data & xbit)
                CRC32 ^= dwPolynomial;
            xbit >>= 1;
        }
    }
    return CRC32;
}

// ============================================================================
// 饱和限幅
// ============================================================================
static inline float saturate(float val, float min, float max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// ============================================================================
// 构造
// ============================================================================
UnitreeMotor::UnitreeMotor(const Config &cfg)
    : cfg_(cfg), fb_{}
{
}

// ============================================================================
// 公共接口
// ============================================================================
bool UnitreeMotor::sendRecv(const Command &cmd)
{
    if (cfg_.type == GO_M8010_6)
        return sendRecv_GO(cmd);
    else
        return sendRecv_A1(cmd);
}

bool UnitreeMotor::enable()
{
    Command cmd = {0, 0, 0, 0, 0};
    // GO: status=1 (FOC), A1: mode=10 (FOC)
    // sendRecv_xx 内部根据 enable 设置 mode 字段
    // 这里用一个特殊手法：发送零指令但 mode=FOC
    if (cfg_.type == GO_M8010_6)
        return sendRecv_GO(cmd);   // status=1 在 sendRecv_GO 内设置
    else
        return sendRecv_A1(cmd);   // mode=10 在 sendRecv_A1 内设置
}

bool UnitreeMotor::disable()
{
    // 发送 mode=0 停止帧
    if (cfg_.type == GO_M8010_6) {
        // GO: 构建 status=0 的帧
        uint8_t tx[17];
        memset(tx, 0, sizeof(tx));
        tx[0] = 0xFE;
        tx[1] = 0xEE;
        tx[2] = (cfg_.motor_id & 0x0F) << 4;  // status=0
        uint16_t crc = crc_ccitt(0, tx, 15);
        tx[15] = crc & 0xFF;
        tx[16] = (crc >> 8) & 0xFF;
        HAL_UART_Transmit(cfg_.huart, tx, 17, 5);
        return true;
    } else {
        // A1: mode=0
        uint8_t tx[34];
        memset(tx, 0, sizeof(tx));
        tx[0] = 0xFE;
        tx[1] = 0xEE;
        tx[2] = cfg_.motor_id;
        // mode=0, 其余全零
        tx[5] = 0xFF;  // ModifyBit
        uint32_t crc = crc32_core((uint32_t *)tx, 7);
        memcpy(&tx[28], &crc, 4);
        // 修正：A1 CRC 在 [30-33]
        memset(&tx[28], 0, 2);  // Res 的 LowHzMotorCmdIndex/Byte 之后
        // 直接按结构体偏移写 CRC
        uint32_t *p = (uint32_t *)tx;
        uint32_t c = crc32_core(p, 7);
        tx[30] = c & 0xFF;
        tx[31] = (c >> 8) & 0xFF;
        tx[32] = (c >> 16) & 0xFF;
        tx[33] = (c >> 24) & 0xFF;
        HAL_UART_Transmit(cfg_.huart, tx, 34, 5);
        return true;
    }
}

// ============================================================================
// GO-M8010-6 协议
// ============================================================================

// GO TX 帧结构 (17 bytes, packed)
#pragma pack(push, 1)
struct GO_TxFrame {
    uint8_t  head[2];       // 0xFE, 0xEE
    uint8_t  mode;          // id(4bit) | status(3bit) | none(1bit)
    int16_t  tor_des;       // T * 256
    int16_t  spd_des;       // W / 6.2832 * 256
    int32_t  pos_des;       // Pos / 6.2832 * 32768
    int16_t  k_pos;         // K_P / 25.6 * 32768
    int16_t  k_spd;         // K_W / 25.6 * 32768
    uint16_t crc16;
};

struct GO_RxFrame {
    uint8_t  head[2];       // 0xFD, 0xEE
    uint8_t  mode;          // id(4bit) | status(3bit) | none(1bit)
    int16_t  torque;
    int16_t  speed;
    int32_t  pos;
    int8_t   temp;
    uint8_t  MError_force[3];  // MError(3bit) + force(12bit) + none(1bit)
    uint16_t crc16;
};
#pragma pack(pop)

bool UnitreeMotor::sendRecv_GO(const Command &cmd)
{
    GO_TxFrame tx;
    tx.head[0] = 0xFE;
    tx.head[1] = 0xEE;

    // mode: id(高4bit) | status(中3bit) | none(低1bit)
    // status=1 为 FOC 闭环
    tx.mode = ((cfg_.motor_id & 0x0F) << 4) | (1 << 1);  // status=1

    float T   = saturate(cmd.T,   -127.99f, 127.99f);
    float W   = saturate(cmd.W,   -804.0f,  804.0f);
    float Pos = saturate(cmd.Pos, -411774.0f, 411774.0f);
    float K_P = saturate(cmd.K_P, 0.0f, 25.599f);
    float K_W = saturate(cmd.K_W, 0.0f, 25.599f);

    tx.tor_des = (int16_t)(T * 256.0f);
    tx.spd_des = (int16_t)(W / 6.2832f * 256.0f);
    tx.pos_des = (int32_t)(Pos / 6.2832f * 32768.0f);
    tx.k_pos   = (int16_t)(K_P / 25.6f * 32768.0f);
    tx.k_spd   = (int16_t)(K_W / 25.6f * 32768.0f);

    tx.crc16 = crc_ccitt(0, (uint8_t *)&tx, 15);

    // 发送
    if (HAL_UART_Transmit(cfg_.huart, (uint8_t *)&tx, 17, 5) != HAL_OK)
        return false;

    // 接收
    GO_RxFrame rx;
    uint16_t rx_len = 0;
    if (HAL_UARTEx_ReceiveToIdle(cfg_.huart, (uint8_t *)&rx, sizeof(rx), &rx_len, 5) != HAL_OK)
        return false;

    if (rx_len != 16 || rx.head[0] != 0xFD || rx.head[1] != 0xEE) {
        fb_.valid = false;
        return false;
    }

    // CRC 校验 (前 14 字节)
    uint16_t crc_check = crc_ccitt(0, (uint8_t *)&rx, 14);
    if (crc_check != rx.crc16) {
        fb_.valid = false;
        return false;
    }

    // 解码
    fb_.T    = (float)rx.torque / 256.0f;
    fb_.W    = ((float)rx.speed / 256.0f) * 6.2832f;
    fb_.Pos  = 6.2832f * (float)rx.pos / 32768.0f;
    fb_.Temp = rx.temp;
    fb_.MError = rx.MError_force[0] >> 5;  // 高 3 bit
    fb_.valid = true;

    return true;
}

// ============================================================================
// A1 协议
// ============================================================================

#pragma pack(push, 1)

typedef union {
    int32_t  L;
    uint8_t  u8[4];
    uint16_t u16[2];
    uint32_t u32;
    float    F;
} COMData32;

struct A1_TxFrame {
    // head (4 bytes)
    uint8_t  start[2];      // 0xFE, 0xEE
    uint8_t  motorID;
    uint8_t  reserved0;
    // Mdata (26 bytes)
    uint8_t  mode;           // 10=FOC, 0=idle
    uint8_t  ModifyBit;      // 0xFF
    uint8_t  ReadBit;        // 0
    uint8_t  reserved1;
    COMData32 Modify;        // 4 bytes, =0
    int16_t  T;              // T * 256
    int16_t  W;              // W * 128
    int32_t  Pos;            // (Pos / 16.2832) * 16384
    int16_t  K_P;            // K_P * 78.5577
    int16_t  K_W;            // K_W * 102400
    uint8_t  LowHzMotorCmdIndex;
    uint8_t  LowHzMotorCmdByte;
    COMData32 Res;           // 4 bytes, =0
    // CRC (4 bytes)
    COMData32 CRCdata;
};

struct A1_RxFrame {
    // head (4 bytes)
    uint8_t  start[2];
    uint8_t  motorID;
    uint8_t  reserved0;
    // Mdata (70 bytes)
    uint8_t  mode;
    uint8_t  ReadBit;
    int8_t   Temp;
    uint8_t  MError;
    COMData32 Read;
    int16_t  T;
    int16_t  W;
    float    LW;             // 关节侧实际速度 (rad/s)
    int16_t  W2;
    float    LW2;
    int16_t  Acc;
    int16_t  OutAcc;
    int32_t  Pos;
    int32_t  Pos2;
    int16_t  gyro[3];
    int16_t  acc[3];
    int16_t  Fgyro[3];
    int16_t  Facc[3];
    int16_t  Fmag[3];
    uint8_t  Ftemp;
    int16_t  Force16;
    int8_t   Force8;
    uint8_t  FError;
    int8_t   Res[1];
    // CRC (4 bytes)
    COMData32 CRCdata;
};

#pragma pack(pop)

bool UnitreeMotor::sendRecv_A1(const Command &cmd)
{
    A1_TxFrame tx;
    memset(&tx, 0, sizeof(tx));

    tx.start[0] = 0xFE;
    tx.start[1] = 0xEE;
    tx.motorID  = cfg_.motor_id;

    tx.mode      = 10;    // FOC
    tx.ModifyBit = 0xFF;

    tx.T   = (int16_t)(saturate(cmd.T, -127.99f, 127.99f) * 256.0f);
    tx.W   = (int16_t)(saturate(cmd.W, -804.0f, 804.0f) * 128.0f);
    tx.Pos = (int32_t)(cmd.Pos / 16.2832f * 16384.0f);
    tx.K_P = (int16_t)(saturate(cmd.K_P, 0.0f, 25.599f) * 78.5577f);
    tx.K_W = (int16_t)(saturate(cmd.K_W, 0.0f, 25.599f) * 102400.0f);

    // CRC32: 前 7 个 uint32_t = 28 字节 (head + Mdata 前 24 字节)
    tx.CRCdata.u32 = crc32_core((uint32_t *)&tx, 7);

    // 发送
    if (HAL_UART_Transmit(cfg_.huart, (uint8_t *)&tx, 34, 5) != HAL_OK)
        return false;

    // 接收
    A1_RxFrame rx;
    uint16_t rx_len = 0;
    if (HAL_UARTEx_ReceiveToIdle(cfg_.huart, (uint8_t *)&rx, sizeof(rx), &rx_len, 10) != HAL_OK)
        return false;

    if (rx_len != 78) {
        fb_.valid = false;
        return false;
    }

    // CRC32 校验: 前 18 个 uint32_t = 72 字节
    if (rx.CRCdata.u32 != crc32_core((uint32_t *)&rx, 18)) {
        fb_.valid = false;
        return false;
    }

    // 解码
    fb_.T      = (float)rx.T / 256.0f;
    fb_.W      = rx.LW;              // 关节侧速度直接用浮点字段
    fb_.Pos    = 6.2832f * (float)rx.Pos / 16384.0f;
    fb_.Temp   = rx.Temp;
    fb_.MError = rx.MError;
    fb_.valid  = true;

    return true;
}
