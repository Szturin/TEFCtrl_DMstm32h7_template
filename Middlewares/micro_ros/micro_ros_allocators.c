/**
 * @file micro_ros_allocators.c
 * @brief micro-ROS 内存/时钟 适配层（使用 RT-Thread heap）
 *
 * micro-ROS 内部所有动态分配都通过此文件的 4 个函数进行，
 * 替换为 RT-Thread 的 rt_malloc/rt_free 即可无缝集成。
 */

#include "rtthread.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* 内存分配（对应 rcl_allocator_t 的 4 个函数指针）                     */
/* ------------------------------------------------------------------ */

void *microros_allocate(size_t size, void *state)
{
    (void)state;
    return rt_malloc(size);
}

void microros_deallocate(void *pointer, void *state)
{
    (void)state;
    rt_free(pointer);
}

void *microros_reallocate(void *pointer, size_t size, void *state)
{
    (void)state;
    return rt_realloc(pointer, size);
}

void *microros_zero_allocate(size_t num, size_t size, void *state)
{
    (void)state;
    void *p = rt_malloc(num * size);
    if (p)
        memset(p, 0, num * size);
    return p;
}

/* ------------------------------------------------------------------ */
/* 时钟（micro-ROS 需要纳秒级时间戳）                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief 提供给 rmw_uros_set_custom_transport 前需设置时钟函数。
 *        若使用 rcutils 时钟，可注册此函数：
 *          rcutils_set_default_allocator(...);
 *
 * RT-Thread tick → ms → ns 转换（RT_TICK_PER_SECOND = 1000 时，1 tick = 1ms）
 */
int64_t microros_get_time_ns(void)
{
    return (int64_t)rt_tick_get() * (1000000LL);  /* ms → ns */
}
