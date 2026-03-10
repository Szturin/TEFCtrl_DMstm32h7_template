#pragma once

/* ================================================================
 * app_link.h — 应用层统一链接树
 *
 * C / C++ 文件均可 include，C++ 专有部分在 __cplusplus 块内。
 * 依赖方向：标准库 → HAL → bsp → modules → task
 * ================================================================ */

/* --- 标准库 --- */
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* --- HAL 硬件层 --- */
#include "stm32h7xx_hal.h"
#include "main.h"
#include "memorymap.h"
#include "usart.h"
#include "gpio.h"

/* --- BSP 层 --- */
#include "bsp/can/bsp_fdcan.h"
#include "bsp/delay/delay.h"
#include "bsp/dwt/bsp_dwt.h"

/* --- Modules 层 --- */
#include "modules/remote/remoter_uart.h"
#include "modules/ringbuffer/ringbuffer.h"
#include "modules/daemon/daemon.h"

/* --- C++ 专用（含 class 的头文件）--- */
#ifdef __cplusplus
#include "bsp/usart/bsp_usart.h"
#include "task/simple_os/scheduler.h"
#include "task/motor_task.h"
#include "task/uart_task.h"
#endif
