/**
 * @file micro_ros_transport.c
 * @brief micro-ROS UART transport 适配层（STM32H7 HAL + RT-Thread）
 *
 * TX：HAL_UART_Transmit（阻塞，带超时），简单可靠
 * RX：UART IDLE 中断 + DMA 循环缓冲 + RT-Thread 信号量通知
 *
 * 移植步骤（CubeMX 侧）：
 *   1. 启用 USARTx（本文件默认 USART3）
 *   2. 启用 DMA：USART3_RX → DMA1 Stream X（Circular 模式）
 *   3. 启用 USART3 全局中断（处理 IDLE 中断）
 *   4. 在 stm32h7xx_it.c 的 USARTx_IRQHandler 中调用：
 *        microros_uart_rxcplt_callback();
 */

#include "micro_ros_transport.h"
#include "rtthread.h"
#include "stm32h7xx_hal.h"

/* 外部 UART 句柄（CubeMX 生成） */
extern UART_HandleTypeDef MICROROS_UART_HANDLE;

/* ---- 接收环形缓冲（无锁单消费者，中断写入 / 线程读取）---- */
static uint8_t  rx_dma_buf[MICROROS_RX_BUF_SIZE];   /* DMA 循环缓冲 */
static uint8_t  rx_ring[MICROROS_RX_BUF_SIZE];       /* 应用层环形缓冲 */
static uint16_t rx_head = 0;   /* 写指针（中断更新） */
static uint16_t rx_tail = 0;   /* 读指针（线程更新） */
static uint16_t rx_dma_last = 0;

/* RT-Thread 信号量：有新数据时由中断发出 */
static struct rt_semaphore rx_sem;

/* ------------------------------------------------------------------ */
/* 内部工具                                                             */
/* ------------------------------------------------------------------ */

/* DMA 当前写入位置 */
static inline uint16_t dma_write_pos(void)
{
    return (uint16_t)(MICROROS_RX_BUF_SIZE -
                      __HAL_DMA_GET_COUNTER(MICROROS_UART_HANDLE.hdmarx));
}

/* 将 DMA 新数据搬入应用环形缓冲 */
static void flush_dma_to_ring(void)
{
    uint16_t pos = dma_write_pos();
    while (rx_dma_last != pos)
    {
        rx_ring[rx_head] = rx_dma_buf[rx_dma_last];
        rx_head = (rx_head + 1) % MICROROS_RX_BUF_SIZE;
        rx_dma_last = (rx_dma_last + 1) % MICROROS_RX_BUF_SIZE;
    }
}

/* ------------------------------------------------------------------ */
/* UART IDLE 中断回调（在 stm32h7xx_it.c 的 IRQHandler 内调用）        */
/* ------------------------------------------------------------------ */
void microros_uart_rxcplt_callback(void)
{
    if (__HAL_UART_GET_FLAG(&MICROROS_UART_HANDLE, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&MICROROS_UART_HANDLE);
        flush_dma_to_ring();
        rt_sem_release(&rx_sem);  /* 通知等待线程 */
    }
}

/* ------------------------------------------------------------------ */
/* Transport 回调实现                                                   */
/* ------------------------------------------------------------------ */

bool microros_transport_open(struct uxrCustomTransport *transport)
{
    (void)transport;

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "mros_rx", 0, RT_IPC_FLAG_FIFO);

    /* 启动 DMA 接收（循环模式） */
    rx_head = rx_tail = rx_dma_last = 0;
    HAL_UART_Receive_DMA(&MICROROS_UART_HANDLE, rx_dma_buf, MICROROS_RX_BUF_SIZE);

    /* 使能 UART IDLE 中断 */
    __HAL_UART_ENABLE_IT(&MICROROS_UART_HANDLE, UART_IT_IDLE);

    return true;
}

bool microros_transport_close(struct uxrCustomTransport *transport)
{
    (void)transport;
    __HAL_UART_DISABLE_IT(&MICROROS_UART_HANDLE, UART_IT_IDLE);
    HAL_UART_DMAStop(&MICROROS_UART_HANDLE);
    rt_sem_detach(&rx_sem);
    return true;
}

size_t microros_transport_write(struct uxrCustomTransport *transport,
                                 const uint8_t *buf, size_t len, uint8_t *err)
{
    (void)transport;
    HAL_StatusTypeDef ret = HAL_UART_Transmit(
        &MICROROS_UART_HANDLE, (uint8_t *)buf, (uint16_t)len,
        MICROROS_TX_TIMEOUT_MS);
    if (ret != HAL_OK)
    {
        *err = 1;
        return 0;
    }
    return len;
}

size_t microros_transport_read(struct uxrCustomTransport *transport,
                                uint8_t *buf, size_t len, int timeout_ms,
                                uint8_t *err)
{
    (void)transport;

    /* 等待数据到来（超时单位 ms，RT_TICK_PER_SECOND = 1000） */
    rt_tick_t ticks = (timeout_ms > 0)
                      ? (rt_tick_t)timeout_ms
                      : RT_WAITING_NO;
    rt_sem_take(&rx_sem, ticks);

    /* 搬运 DMA 新数据（双重保险：信号量可能在中断之后才处理）*/
    flush_dma_to_ring();

    /* 从环形缓冲取数据 */
    size_t count = 0;
    while (count < len && rx_tail != rx_head)
    {
        buf[count++] = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1) % MICROROS_RX_BUF_SIZE;
    }

    if (count == 0)
    {
        *err = 1;
    }
    return count;
}
