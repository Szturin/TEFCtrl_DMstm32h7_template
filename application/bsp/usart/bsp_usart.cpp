//
// Created by 123 on 2025/4/27.
//
#include "bsp_usart.h"

Uart_Instance::Uart_Instance(UART_HandleTypeDef *id, bool en)
        :huart(id),initialized(en){
    init();
}


