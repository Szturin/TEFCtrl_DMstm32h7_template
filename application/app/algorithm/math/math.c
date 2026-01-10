//
// Created by 123 on 2025/3/13.
//

#include "math.h"

/**
 * @brief  将一个整型值从一个范围线性映射到另一个范围
 * @param  value: 输入值 (整数)
 * @param  from_min: 原始范围的最小值
 * @param  from_max: 原始范围的最大值
 * @param  to_min: 目标范围的最小值
 * @param  to_max: 目标范围的最大值
 * @return 映射后的值 (整数)
 */
int map_int(int value, int from_min, int from_max, int to_min, int to_max)
{
    if (from_max == from_min)
    {
        // 避免除 0 错误的情况s
        return to_min;
    }
    // 线性映射公式
    return (value - from_min) * (to_max - to_min) / (from_max - from_min) + to_min;
}