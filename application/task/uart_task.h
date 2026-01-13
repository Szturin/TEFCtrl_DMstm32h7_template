#ifndef UART_TASK_H
#define UART_TASK_H

#ifdef __cplusplus
extern "C" {
#endif
#include "bsp/usart/bsp_usart.h"
#include "remote/remoter_uart.h"

#define  BUFFER_SIZE (256)
#define  BUFFER_SIZE_SMALL (64)
#define  BUFFER_SIZE_LARGE (512)


#define WBUS_BUFLEN  (25)

void UartTask(void);
extern uint8_t uart5_rx_dma_buffer[WBUS_BUFLEN];
void uart_ringbuffer_init(void);

#ifdef __cplusplus
}
#endif
#endif // UART_TASK_H
