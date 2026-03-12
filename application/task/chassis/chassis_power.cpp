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
#define K_TORQUE              0.0005f   // PID output → 输出轴扭矩 (N·m)
#define POWER_K1_INIT         0.001067f // 摩擦损耗 (RLS 标定值)
#define POWER_K2_INIT         1.187571f // 铜损 (RLS 标定值)
#define POWER_K3              3.0f      // 常数损耗 (W)，4轮分摊

// 功率限制参数
#define POWER_LIMIT_DEFAULT   80.0f   // 默认功率上限 (W)
#define POWER_OFFSET_DEFAULT  24.0f   // 功率计偏移（裁判系统静态功率）

// 大P分配：error 置信度阈值
#define ERROR_LOWER           1.0f    // Σ|error| < 此值 → 纯等比缩放 (K_coe=0)
#define ERROR_UPPER           10.0f   // Σ|error| > 此值 → 纯大P分配 (K_coe=1)

// RLS 参数（取消注释 RLS_ENABLED 启用在线自适应）
// #define RLS_ENABLED
#define RLS_LAMBDA            0.99999f // 遗忘因子
#define RLS_P_INIT            100.0f   // 协方差矩阵初始对角值
#define RLS_K_MIN             1e-5f    // 参数下限保护

// ===================== 状态变量 =====================

static PowerMeter_t *powermeter = nullptr;
static float power_limit = POWER_LIMIT_DEFAULT;
static float power_offset = POWER_OFFSET_DEFAULT;
static bool power_limit_en = true;
static float estimated_power = 0;
static float measured_power = 0;

// 功率模型参数（RLS 标定后固定，取消注释 RLS_ENABLED 可重新在线标定）
static float power_k1 = POWER_K1_INIT;
static float power_k2 = POWER_K2_INIT;

#ifdef RLS_ENABLED
static float rls_P[2][2];   // 协方差矩阵 2×2
static bool  rls_enabled = true;

static void rls_reset(void) {
    power_k1 = POWER_K1_INIT;
    power_k2 = POWER_K2_INIT;
    rls_P[0][0] = RLS_P_INIT; rls_P[0][1] = 0;
    rls_P[1][0] = 0;          rls_P[1][1] = RLS_P_INIT;
}
#endif

// ===================== 初始化 =====================

void chassis_power_init(FDCAN_HandleTypeDef *hfdcan) {
    powermeter = PowerMeter_Init(hfdcan);
#ifdef RLS_ENABLED
    rls_reset();
#endif
}

// ===================== 单轮功率估算 =====================

static inline float estimate_motor_power(float output, float omega) {
    float torque = output * K_TORQUE;
    return torque * omega + power_k1 * fabsf(omega) + power_k2 * torque * torque + POWER_K3 / 4.0f;
}

// ===================== 功率限制 =====================

void chassis_power_limit(int16_t current[4],
                         const float speed_set[4],
                         const float speed_actual[4])
{
    // 实测功率（减去偏移）
    measured_power = (powermeter != nullptr) ? (powermeter->power - power_offset) : 0;

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

    // ---- RLS 在线自适应更新 k1, k2 ----
#ifdef RLS_ENABLED
    if (rls_enabled && powermeter != nullptr && fabsf(measured_power) > 5.0f) {
        float phi0 = 0, phi1 = 0, sum_tw = 0;
        for (int i = 0; i < 4; i++) {
            float torque = (float)current[i] * K_TORQUE;
            phi0 += fabsf(speed_actual[i]);
            phi1 += torque * torque;
            sum_tw += torque * speed_actual[i];
        }
        float y = measured_power - sum_tw - POWER_K3;
        float y_hat = power_k1 * phi0 + power_k2 * phi1;
        float e = y - y_hat;

        float Pp0 = rls_P[0][0] * phi0 + rls_P[0][1] * phi1;
        float Pp1 = rls_P[1][0] * phi0 + rls_P[1][1] * phi1;
        float denom = RLS_LAMBDA + phi0 * Pp0 + phi1 * Pp1;
        if (fabsf(denom) < 1e-10f) denom = 1e-10f;
        float inv_denom = 1.0f / denom;
        float K0 = Pp0 * inv_denom;
        float K1 = Pp1 * inv_denom;
        power_k1 += K0 * e;
        power_k2 += K1 * e;
        if (power_k1 < RLS_K_MIN) power_k1 = RLS_K_MIN;
        if (power_k2 < RLS_K_MIN) power_k2 = RLS_K_MIN;
        float inv_lambda = 1.0f / RLS_LAMBDA;
        float new_P00 = (rls_P[0][0] - K0 * phi0 * rls_P[0][0] - K0 * phi1 * rls_P[1][0]) * inv_lambda;
        float new_P01 = (rls_P[0][1] - K0 * phi0 * rls_P[0][1] - K0 * phi1 * rls_P[1][1]) * inv_lambda;
        float new_P10 = (rls_P[1][0] - K1 * phi0 * rls_P[0][0] - K1 * phi1 * rls_P[1][0]) * inv_lambda;
        float new_P11 = (rls_P[1][1] - K1 * phi0 * rls_P[0][1] - K1 * phi1 * rls_P[1][1]) * inv_lambda;
        rls_P[0][0] = new_P00; rls_P[0][1] = new_P01;
        rls_P[1][0] = new_P10; rls_P[1][1] = new_P11;
    }
#endif
}

// ===================== 查询接口 =====================

float chassis_power_get_estimated(void) { return estimated_power; }
float chassis_power_get_measured(void)  { return measured_power; }
float chassis_power_get_limit(void)     { return power_limit; }
int   chassis_power_is_enabled(void)    { return power_limit_en ? 1 : 0; }
float chassis_power_get_k1(void)        { return power_k1; }
float chassis_power_get_k2(void)        { return power_k2; }

// ===================== 命令解析 =====================

int chassis_power_cmd_parse(const char *cmd) {
    if (strncmp(cmd, "plimit:", 7) != 0) return 0;

    const char *arg = cmd + 7;
    if (strcmp(arg, "off") == 0) {
        power_limit_en = false;
    } else if (strcmp(arg, "on") == 0) {
        power_limit_en = true;
    } else if (strncmp(arg, "offset:", 7) == 0) {
        power_offset = (float)atof(arg + 7);
#ifdef RLS_ENABLED
    } else if (strcmp(arg, "rls:on") == 0) {
        rls_enabled = true;
    } else if (strcmp(arg, "rls:off") == 0) {
        rls_enabled = false;
    } else if (strcmp(arg, "rls:reset") == 0) {
        rls_reset();
#endif
    } else {
        float val = (float)atof(arg);
        if (val > 0) {
            power_limit = val;
            power_limit_en = true;
        }
    }
    return 1;
}
