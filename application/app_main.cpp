/**
 * @file app_main.cpp
 * @brief 应用层 C++ 入口，由 main.c 的 USER CODE 调用
 *
 * CubeMX 重新生成代码时只会修改 main.c，本文件不受影响。
 * 所有 C++ 对象、BSP 初始化、任务注册均在此文件中管理。
 */
#include "app_main.h"
#include "main.h"
#include "bsp/usart/bsp_usart.h"
#include "bsp/can/bsp_fdcan.h"
#include "task/simple_os/scheduler.h"
#include "task/motor_task.h"
#include "task/uart_task.h"

// ==============================================================================
// 全局 C++ 对象（其他文件通过 extern 访问）
// ==============================================================================
Uart_Instance uart_log(&huart7, true);
Scheduler os;

// ==============================================================================
// 应用初始化（对应原 main.c USER CODE BEGIN 2）
// ==============================================================================
void app_main_init(void) {
    HAL_Delay(100);

    uart_log.init();
    uart_log.printf("\r\n System Init.\r\n");

    bsp_fdcan_set_baud(&hfdcan1, CAN_CLASS, CAN_BR_1M);
    bsp_can_init();

    uart_ringbuffer_init();
    RemoteInit();
    HAL_UARTEx_ReceiveToIdle_DMA(&huart5, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));
    __HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_HT);

    os.init();
    motor_task_init();
    shoot_task_init();
    os.addTask(motor_task_proc, 5, 1);
    os.addTask(shoot_task_proc, 5, 2);
    os.addTask(UartTask,        5, 3);

    HAL_Delay(500);
}

// ==============================================================================
// 主循环（对应原 main.c while(1) 内部）
// ==============================================================================
void app_main_run(void) {
    os.run();
}
