/**
 * @file  chassis_power.cpp
 * @brief 底盘功率控制（模型前馈 + 大P分配 + 负功率处理）
 *
 * 参考 HKUST ENTERPRIZE RM2024-PowerModule 开源方案简化实现：
 * - 电机模型前馈估算功率（不依赖功率计延迟反馈）
 * - 大P分配：error 大时按误差分配，error 小时按指令功率等比缩放
 * - 负功率电机（制动回馈）不参与分配，回馈功率返还池子
 */

#include "chassis_power.h"
#include "powermeter/powermeter.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ===================== 功率模型参数 =====================

// P_i = τ_i·Ω_i + k1·|Ω_i| + k2·τ_i² + k3/4
// τ_i = output_i * K_TORQUE
// M3508: KT_motor ≈ 0.02 N·m/A, ratio=19, C620: ±16384→±20A
// K_TORQUE = (20/16384) * 0.02 * 19 ≈ 0.000464
#define K_TORQUE              0.0005f // PID output → 输出轴扭矩 (N·m)
#define POWER_K1              0.20f   // 摩擦损耗 (W/(rad/s))
#define POWER_K2              1.2f    // 铜损 (W/(N·m)²)
#define POWER_K3              3.0f    // 常数损耗 (W)，4轮分摊

// 功率限制参数
#define POWER_LIMIT_DEFAULT   80.0f   // 默认功率上限 (W)

// 大P分配：error 置信度阈值
#define ERROR_LOWER           1.0f    // Σ|error| < 此值 → 纯等比缩放 (K_coe=0)
#define ERROR_UPPER           10.0f   // Σ|error| > 此值 → 纯大P分配 (K_coe=1)

// ===================== 状态变量 =====================

static PowerMeter_t *powermeter = nullptr;
static float power_limit = POWER_LIMIT_DEFAULT;
static bool power_limit_en = true;
static float estimated_power = 0;
static float measured_power = 0;

// ===================== 初始化 =====================

void chassis_power_init(FDCAN_HandleTypeDef *hfdcan) {
    powermeter = PowerMeter_Init(hfdcan);
}

// ===================== 单轮功率估算 =====================

static inline float estimate_motor_power(float output, float omega) {
    float torque = output * K_TORQUE;
    return torque * omega + POWER_K1 * fabsf(omega) + POWER_K2 * torque * torque + POWER_K3 / 4.0f;
}

// ===================== 功率限制 =====================

void chassis_power_limit(int16_t current[4],
                         const float speed_set[4],
                         const float speed_actual[4])
{
    // 实测功率（VOFA+ 对比用）
    measured_power = (powermeter != nullptr) ? powermeter->power : 0;

    if (!power_limit_en) {
        // 不限功率时仍更新估算值
        estimated_power = 0;
        for (int i = 0; i < 4; i++)
            estimated_power += estimate_motor_power((float)current[i], speed_actual[i]);
        return;
    }

    // 1. 估算各轮功率
    float pcmd[4], error_abs[4];
    float sum_cmd_power = 0;

    for (int i = 0; i < 4; i++) {
        pcmd[i] = estimate_motor_power((float)current[i], speed_actual[i]);
        error_abs[i] = fabsf(speed_set[i] - speed_actual[i]);
        sum_cmd_power += pcmd[i];
    }
    estimated_power = sum_cmd_power;

    // 不超功率，放行
    if (sum_cmd_power <= power_limit) return;

    // 2. 分离正功率/负功率电机
    float allocatable = power_limit;
    float sum_positive = 0;
    float sum_error = 0;

    for (int i = 0; i < 4; i++) {
        if (pcmd[i] <= 0.0f) {
            allocatable += -pcmd[i];  // 回馈功率返还池
        } else {
            sum_positive += pcmd[i];
            sum_error += error_abs[i];
        }
    }

    if (sum_positive < 0.01f) return;

    // 3. error 置信度
    float k_coe;
    if (sum_error <= ERROR_LOWER)      k_coe = 0.0f;
    else if (sum_error >= ERROR_UPPER) k_coe = 1.0f;
    else k_coe = (sum_error - ERROR_LOWER) / (ERROR_UPPER - ERROR_LOWER);

    // 4. 大P分配 + 电流缩放
    for (int i = 0; i < 4; i++) {
        if (pcmd[i] <= 0.0f) continue;

        float w_error = (sum_error    > 0.01f) ? (error_abs[i] / sum_error)    : 0.25f;
        float w_cmd   = (sum_positive > 0.01f) ? (pcmd[i]      / sum_positive) : 0.25f;
        float k_i = k_coe * w_error + (1.0f - k_coe) * w_cmd;

        float p_budget = k_i * allocatable;
        float ratio = p_budget / pcmd[i];
        if (ratio < 1.0f) {
            current[i] = (int16_t)((float)current[i] * ratio);
        }
    }
}

// ===================== 查询接口 =====================

float chassis_power_get_estimated(void) { return estimated_power; }
float chassis_power_get_measured(void)  { return measured_power; }
float chassis_power_get_limit(void)     { return power_limit; }
int   chassis_power_is_enabled(void)    { return power_limit_en ? 1 : 0; }

// ===================== 命令解析 =====================

int chassis_power_cmd_parse(const char *cmd) {
    if (strncmp(cmd, "plimit:", 7) != 0) return 0;

    const char *arg = cmd + 7;
    if (strcmp(arg, "off") == 0) {
        power_limit_en = false;
    } else if (strcmp(arg, "on") == 0) {
        power_limit_en = true;
    } else {
        float val = (float)atof(arg);
        if (val > 0) {
            power_limit = val;
            power_limit_en = true;
        }
    }
    return 1;
}
