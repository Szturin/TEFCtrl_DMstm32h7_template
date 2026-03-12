/**
 * @file  chassis_power.h
 * @brief 底盘功率控制（模型前馈 + 大P分配）
 *
 * 功率估算模型: P_i = τ_i·Ω_i + k1·|Ω_i| + k2·τ_i² + k3/4
 * 分配策略: K_coe 混合 error-proportional 与 command-proportional
 * 负功率处理: 制动回馈电机不参与分配，功率返还池子
 */
#ifndef CHASSIS_POWER_H
#define CHASSIS_POWER_H

#include "bsp/can/bsp_fdcan.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化功率控制模块（包括功率计）
 * @param hfdcan 功率计所在的 FDCAN 总线
 */
void chassis_power_init(FDCAN_HandleTypeDef *hfdcan);

/**
 * @brief 功率限制：对 PID 输出电流做前馈功率分配
 * @param current      [in/out] 4 轮电流指令，超功率时会被缩放
 * @param speed_set    [in]     4 轮目标速度 (rad/s)
 * @param speed_actual [in]     4 轮实际速度 (rad/s)
 */
void chassis_power_limit(int16_t current[4],
                         const float speed_set[4],
                         const float speed_actual[4]);

/** 获取模型估算总功率 (W) */
float chassis_power_get_estimated(void);

/** 获取功率计实测总功率 (W) */
float chassis_power_get_measured(void);

/** 获取当前功率上限 (W) */
float chassis_power_get_limit(void);

/** 获取功率限制使能状态 */
int chassis_power_is_enabled(void);

/** 获取 RLS 自适应后的 k1 (摩擦损耗系数) */
float chassis_power_get_k1(void);

/** 获取 RLS 自适应后的 k2 (铜损系数) */
float chassis_power_get_k2(void);

/**
 * @brief 解析功率相关 VOFA+ 命令
 * @return 1=已处理, 0=不是功率命令
 */
int chassis_power_cmd_parse(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_POWER_H
