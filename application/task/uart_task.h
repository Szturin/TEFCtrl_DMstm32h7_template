#ifndef UART_TASK_H
#define UART_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

void uart_task_init(void);
void uart_task_proc(void);

#ifdef __cplusplus
}
#endif

#endif // UART_TASK_H
