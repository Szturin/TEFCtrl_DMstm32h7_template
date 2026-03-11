#ifndef USART_TASK_H
#define USART_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32h7xx_hal.h"

#define WBUS_BUFLEN (25)

void uart_ringbuffer_init(void);
void usart_task_proc(void);

extern uint8_t uart5_rx_dma_buffer[WBUS_BUFLEN];

#ifdef __cplusplus
}
#endif

#endif // USART_TASK_H
