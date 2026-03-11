/**
 * @file app_main.cpp
 * @brief 应用层 C++ 入口（RT-Thread 版本）
 *
 * app_main_init() 在 main.c USER CODE BEGIN 2 中调用，完成 BSP 外设初始化。
 * RT-Thread 启动后，各业务线程在此文件中创建。
 * app_main_run() 已不使用（调度由 RT-Thread 内核负责）。
 */
#include "app_main.h"
#include "main.h"
#include "rtthread.h"
#include "bsp/usart/bsp_usart.h"
#include "bsp/can/bsp_fdcan.h"
#include "task/motor_task.h"
#include "task/usart/usart_task.h"
#include "modules/daemon/daemon.h"
#include "task/micro_ros/micro_ros_task.h"
#include "usbd_cdc_if.h"
#include "modules/vofa+/vofa.h"
#include "modules/algorithm/controller/pid_tune.h"

// --- 全局对象 ---

Uart_Instance uart_log(&huart7, true);

// --- 辅助宏 ---。

#define THREAD_START(name, entry, stack, prio) \
    do { \
        rt_thread_t _t = rt_thread_create((name), (entry), RT_NULL, (stack), (prio), 5); \
        RT_ASSERT(_t != RT_NULL); \
        rt_thread_startup(_t); \
    } while (0)

// --- 线程入口 ---

static void motor_thread_entry(void *param){
    (void)param;
    motor_task_init();
    shoot_task_init();
    while (1) {
        motor_task_proc();   // DM 电机
        shoot_task_proc();   // DJI 电机速度环（1ms）
        rt_thread_mdelay(1); // 1ms 底层电机周期
    }
}

static void unitree_thread_entry(void *param){
    (void)param;
    unitree_task_init();
    while (1) {
        unitree_task_proc();
        rt_thread_mdelay(2); // 2ms 周期 (~500Hz)
    }
}

static void uart_thread_entry(void *param){
    (void)param;
    while (1) {
        usart_task_proc();
        rt_thread_mdelay(5);
    }
}

static void robot_thread_entry(void *param){
    (void)param;
    Robot_Init();
    while (1) {

        rt_thread_mdelay(2);
    }
}

static void chassis_thread_entry(void *param){
    (void)param;
    while (1) {
        Chassis_Task();
        rt_thread_mdelay(2);
    }
}
// --- BSP 初始化（rtthread_startup 之前调用） ---

void app_main_init(void){
    rt_thread_mdelay(100);

    uart_log.init();
    uart_log.printf("\r\n[rtt] System BSP Init.\r\n");

    // 跳过 bsp_fdcan_set_baud（CubeMX 已配 1M + CLASSIC），避免 DeInit/ReInit 破坏 Message RAM
    bsp_can_init();

    uart_ringbuffer_init();
    RemoteInit();
    HAL_UARTEx_ReceiveToIdle_DMA(&huart5, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));
    __HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_HT);
}

// --- 线程创建（由 RT-Thread main 线程调用） ---

extern "C" void rtt_app_threads_init(void){
    DaemonStart();

    // VOFA+ 初始化：接收 USB 命令 → pid_tune 解析
    vofa_init();
    vofa_register_cmd_handler([](const char *cmd) {
        PID_UpdateFromCommand(&PID_Test, cmd);
    });

    THREAD_START("motor",  motor_thread_entry,  2048, 10);  // 1ms，最高优先级
    THREAD_START("unitree", unitree_thread_entry, 2048, 11); // 2ms，宇树电机
    THREAD_START("uart",   uart_thread_entry,   2048, 12);
    THREAD_START("robot",   robot_thread_entry,   2048, 12);
    THREAD_START("chassis",  chassis_thread_entry,   2048, 12);
#ifdef MICRO_ROS_ENABLED
    THREAD_START("microros", micro_ros_thread_entry, 8192, 15);
#endif

    uart_log.printf("[rtt] All threads started.\r\n");
    usb_printf("[usb] All threads started.\r\n");
}

// --- 保留接口兼容 ---

void app_main_run(void) {}
