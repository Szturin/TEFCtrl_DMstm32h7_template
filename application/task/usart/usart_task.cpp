#include "usart_task.h"
#include <string.h>
#include "usart.h"
#include "modules/ringbuffer/ringbuffer_test.h"
#include "modules/remote/remoter_uart.h"
#include "bsp/usart/bsp_usart.h"

static ringbuffer_t rb_uart5;  // UART5 环形缓冲区（遥控器）
uint8_t uart5_rx_dma_buffer[WBUS_BUFLEN] = {0};
static uint8_t uart5_read_buffer[WBUS_BUFLEN];
extern Uart_Instance uart_log;

void uart_ringbuffer_init(void) {
    ringbuffer_init(&rb_uart5);  // 遥控器串口环形缓冲区初始化
}

/**
 * @brief UART 空闲中断回调 — 处理 DMA 接收完成的数据
 *
 * @note 当 UART5（遥控器）接收完成或空闲时触发
 *       数据写入环形缓冲区，等待 usart_task_proc() 处理
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == UART5) {  // 遥控器串口
        ringbuffer_write(&rb_uart5, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));
        memset(uart5_rx_dma_buffer, 0, sizeof(uart5_rx_dma_buffer));
        HAL_UARTEx_ReceiveToIdle_DMA(&huart5, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));
        __HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_HT);  // 禁用 DMA 半传输中断
    }
}

/**
 * @brief UART 错误回调 — 处理溢出、奇偶校验、帧错误
 *
 * @note 常见错误：奇偶校验错误、接收溢出、帧错误
 *       此时需要重新启动 DMA 接收
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART5) {  // 遥控器
        // 重新启动 DMA 接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart5, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));
        __HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_HT);
    }
}

/**
 * @brief UART 任务主函数 — 处理所有串口数据
 *
 * @note 由 RT-Thread 串口线程定期调用（周期 5ms）
 *       从缓冲区读取数据，分发到各个模块处理
 */
void usart_task_proc(void) {
    // 处理 UART5（遥控器）
    if (ringbuffer_is_empty(&rb_uart5)) {
        return;
    }

    // 从环形缓冲区读取数据
    ringbuffer_read(&rb_uart5, uart5_read_buffer, sizeof(uart5_read_buffer));

    // 调试输出
    uart_log.printf("%d %d %d %d\r\n", wbus_rc.remote.ch1,
                    wbus_rc.remote.ch2,
                    wbus_rc.remote.ch3,
                    wbus_rc.remote.SH);

    // 解析 WBUS 数据（遥控器协议）
    parse_wbus_data(&wbus_rc, uart5_read_buffer);
    memset(uart5_read_buffer, 0, sizeof(uart5_read_buffer));
}
