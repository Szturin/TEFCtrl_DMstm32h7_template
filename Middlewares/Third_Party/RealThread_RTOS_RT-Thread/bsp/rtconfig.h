/* RT-Thread config file — TEFCtrl STM32H723 rtt */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

/* ---- Cortex-M7 Cache 支持 ---- */
#define RT_USING_CACHE          /* 启用 cpu_cache.c（SCB_EnableICache/DCache） */

/* ---- 基础内核 ---- */
#define RT_THREAD_PRIORITY_MAX    32    /* 线程优先级数量（0=最高，31=最低） */
#define RT_TICK_PER_SECOND       1000  /* SysTick 1ms */
#define RT_ALIGN_SIZE               4
#define RT_NAME_MAX                 8
#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE  1024  /* main线程只做线程创建，1KB足够 */
/* 空闲线程栈：若加 idle_hook（喂狗等）需增大 */
#define RT_IDLE_THREAD_STACK_SIZE   512

/* ---- 调试 ---- */
/* #define RT_DEBUG */
#define RT_DEBUG_INIT               0
#define RT_USING_OVERFLOW_CHECK

/* ---- IPC ---- */
#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* ---- 软件定时器 ---- */
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO        4
#define RT_TIMER_THREAD_STACK_SIZE  1024   /* 512太小：Daemon+其他timer叠加时可能溢出 */

/* ---- 内存管理 ---- */
#define RT_USING_HEAP
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP

/* ---- 控制台 ---- */
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE         256
#define RT_CONSOLE_DEVICE_NAME    "uart1"

/* ---- FinSH 串口 Shell ---- */
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_USING_MSH_ONLY
#define FINSH_THREAD_PRIORITY      20
#define FINSH_THREAD_STACK_SIZE   2048
#define FINSH_HISTORY_LINES         5
#define FINSH_USING_SYMTAB

#endif /* __RTTHREAD_CFG_H__ */
