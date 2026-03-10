/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 */

#include <stdint.h>
#include <rthw.h>
#include <rtthread.h>
#include "stm32h7xx_hal.h"  /* HAL_IncTick / HAL_Init 等 */

#define _SCB_BASE       (0xE000E010UL)
#define _SYSTICK_CTRL   (*(rt_uint32_t *)(_SCB_BASE + 0x0))
#define _SYSTICK_LOAD   (*(rt_uint32_t *)(_SCB_BASE + 0x4))
#define _SYSTICK_VAL    (*(rt_uint32_t *)(_SCB_BASE + 0x8))
#define _SYSTICK_CALIB  (*(rt_uint32_t *)(_SCB_BASE + 0xC))
#define _SYSTICK_PRI    (*(rt_uint8_t  *)(0xE000ED23UL))

// Updates the variable SystemCoreClock and must be called
// whenever the core clock is changed during program execution.
extern void SystemCoreClockUpdate(void);

// Holds the system core clock, which is the system clock
// frequency supplied to the SysTick timer and the processor
// core clock.
extern uint32_t SystemCoreClock;

static uint32_t _SysTick_Config(rt_uint32_t ticks)
{
    if ((ticks - 1) > 0xFFFFFF)
    {
        return 1;
    }

    _SYSTICK_LOAD = ticks - 1;
    _SYSTICK_PRI = 0xFF;
    _SYSTICK_VAL  = 0;
    _SYSTICK_CTRL = 0x07;

    return 0;
}

/* ========================================================================
 * 重写 HAL weak 函数 —— 参考 RT-Thread 官方 drv_common.c
 * HAL_RCC_ClockConfig 内部最后会调用 HAL_InitTick()，
 * 默认实现可能在 RTOS 禁中断环境下失败。
 * 此处让 RT-Thread 完全接管 tick 管理。
 * ======================================================================== */

/**
 * @brief 重写 HAL_InitTick（__weak）
 *        HAL_Init() 和 HAL_RCC_ClockConfig() 内部都会调用此函数。
 *        RT-Thread 自行配置 SysTick，这里直接返回 OK 即可。
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    return HAL_OK;
}

/**
 * @brief 重写 HAL_GetTick（__weak）
 *        在中断禁用期间（rtthread_startup 早期），直接轮询 SysTick COUNTFLAG，
 *        保证 HAL 内部超时机制即使在禁中断时也能正常工作。
 */
uint32_t HAL_GetTick(void)
{
    /* COUNTFLAG（bit16）在 SysTick 计到 0 时置位，读后自动清除 */
    if (_SYSTICK_CTRL & (1 << 16))
    {
        HAL_IncTick();
    }
    return uwTick;
}

/**
 * @brief 重写 HAL_SuspendTick / HAL_ResumeTick（__weak）
 *        RT-Thread 管理 SysTick，禁止 HAL 暂停/恢复。
 */
void HAL_SuspendTick(void) {}
void HAL_ResumeTick(void)  {}

/* ======================================================================== */

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
/* 堆大小：4096 × sizeof(uint32_t) = 16 KB
 * FinSH 内部分配约 2-4KB；IPC 动态对象约 1-2KB；
 * 12 线程静态栈不占堆，但 FinSH 输入缓冲需要动态空间。
 * 建议不低于 8192（32KB），此处取 4096（16KB）作为稳定基准。 */
#define RT_HEAP_SIZE 16384
static uint32_t rt_heap[RT_HEAP_SIZE];     // heap: 16384 * 4 = 64 KB
rt_weak void *rt_heap_begin_get(void)
{
    return rt_heap;
}

rt_weak void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

/* CubeMX-generated init functions (declarations provided by included headers) */
extern void SystemClock_Config(void);
extern void PeriphCommonClock_Config(void);

/**
 * This function will initial your board.
 */
void rt_hw_board_init()
{
    /* HAL 初始化 —— 内部调用 HAL_InitTick() 已被重写为直接返回 OK */
    HAL_Init();

    /* 配置系统时钟（PLL → 480 MHz）
     * HAL_RCC_ClockConfig 内部最后调用 HAL_InitTick()，
     * 已被重写，不再依赖 SysTick 中断，禁中断下也安全。 */
    SystemClock_Config();

    /* 配置外设公共时钟（ADC 等） */
    PeriphCommonClock_Config();

    /* 更新 SystemCoreClock 变量 */
    SystemCoreClockUpdate();

    /* 接管 SysTick 为 RT-Thread 使用（1 kHz） */
    _SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}

void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* 同步 HAL 时基（HAL_GetTick / HAL_Delay 等函数依赖此计数） */
    HAL_IncTick();

    /* leave interrupt */
    rt_interrupt_leave();
}
