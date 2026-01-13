//
// Created by 123 on 2025/4/27.
//

#ifndef TEST_BSP_SYSTEM_H
#define TEST_BSP_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif
/* 嵌入式工程文件链接树*/

/* C语言标准库 */
#include <stdint.h>             // 定义整型
#include <stdio.h>              // 标准输入输出
#include <stdlib.h>             // 标准库
#include <string.h>             // 字符串操作
#include <math.h>               // 数学库

/* 硬件层 */
#include "stm32h7xx_hal.h"
//#include "cmsis_os.h"           // RTOS API
#include "main.h"
#include "stm32h7xx_it.h"
#include "main.h"
#include "memorymap.h"
#include "usart.h"
#include "gpio.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "bsp/can/bsp_fdcan.h"


#include "motor_task.h"
#include "typedef.h"
#include "app/remote/remoter_uart.h"
#include "ringbuffer/ringbuffer.h"
#include "ringbuffer/ringbuffer_test.h"


#ifdef __cplusplus
}
#endif

//C++ 相关
#include "bsp/usart/bsp_usart.h"
#include "task/simple_os/scheduler.h"
#include "uart_task.h"

#endif //TEST_BSP_SYSTEM_H
