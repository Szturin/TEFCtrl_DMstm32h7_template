#include "uart_task.h"
#include "usart.h"
#include "bsp/usart/bsp_usart.h"
#include <stdio.h>
#include <math.h>

extern Uart_Instance uart_log;

static uint32_t uart_task_count = 0;
static float test_value = 0.0f;

// 串口任务初始化
void uart_task_init(void) {
    uart_task_count = 0;
    test_value = 0.0f;
}

// 串口任务处理函数 (5ms周期)
void uart_task_proc(void) {
    uart_task_count++;

    // 每200次执行（1秒）输出一次
    if (uart_task_count % 200 == 0) {
        test_value += 0.1f;
        float sin_value = sinf(test_value);
        uart_log.printf("Task running: %.2f, sin: %.3f\r\n", test_value, sin_value);
    }
}
