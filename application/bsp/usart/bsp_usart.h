//
// Created by 123 on 2025/4/27.
//

#ifndef BSP_USART_H
#define BSP_USART_H

#include "bsp_system.h"
#include "usart.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx.h"
#include "main.h"
#include "memorymap.h"
#include "usart.h"
#include "gpio.h"

#ifdef __cplusplus
#include <cstdarg>
#include <cstring>
#include <cstdio>

class Uart_Instance{
private:
    UART_HandleTypeDef *huart;
    bool initialized;

public:
    Uart_Instance(UART_HandleTypeDef *id, bool en);

    bool init() {

        initialized = true;
        return true;
    }
    bool transmit(const uint8_t *data, uint16_t size, uint32_t timeout){
        HAL_UART_Transmit(huart, data, sizeof(data), timeout);

        return true;// why??
    }
    bool receive(uint8_t* data, uint16_t size, uint32_t timeout = HAL_MAX_DELAY) const;

    int printf(const char *format, ...){
        static char buffer[256];
        va_list arg;
        int len;
        // 初始化可变参数列表
        va_start(arg, format);
        len = vsnprintf(buffer, sizeof(buffer), format, arg);
        va_end(arg);
        HAL_UART_Transmit(huart, (uint8_t *)buffer, (uint16_t)len, 1000);
        return len;
    }
};

extern "C" {
#else
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#endif

// C 接口函数声明（如果需要的话）

#ifdef __cplusplus
}
#endif
#endif //TEST_BSP_USART_H
