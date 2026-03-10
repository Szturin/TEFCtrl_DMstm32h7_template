#ifndef MICRO_ROS_TRANSPORT_H
#define MICRO_ROS_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================
 * micro-ROS UART Transport — STM32H7 + RT-Thread
 *
 * 使用方式：
 *   在 CubeMX 中启用一个 UART（推荐 USART3），开启 DMA TX + RX。
 *   将 MICROROS_UART_HANDLE 指向对应的 UART 句柄。
 *
 * 物理连接（UART → PC / 树莓派 micro-ROS Agent）：
 *   STM32 TX  → USB-TTL → PC
 *   STM32 RX  ← USB-TTL ← PC
 *   波特率：115200 或 460800
 * ============================================================ */

/* ---- 用户配置：改这一行即可切换 UART ---- */
#define MICROROS_UART_HANDLE    huart3       /* CubeMX 生成的句柄名 */
#define MICROROS_UART_BAUDRATE  460800       /* 推荐 460800，低速用 115200 */
#define MICROROS_RX_BUF_SIZE    512          /* 接收环形缓冲区大小 */
#define MICROROS_TX_TIMEOUT_MS  100          /* 发送超时（ms） */

/* ---- Transport 回调（注册给 rmw_uros_set_custom_transport）---- */
bool microros_transport_open (struct uxrCustomTransport *transport);
bool microros_transport_close(struct uxrCustomTransport *transport);
size_t microros_transport_write(struct uxrCustomTransport *transport,
                                 const uint8_t *buf, size_t len, uint8_t *err);
size_t microros_transport_read(struct uxrCustomTransport *transport,
                                uint8_t *buf, size_t len, int timeout, uint8_t *err);

/* ---- 从 UART IDLE 中断中调用（将 DMA 数据写入接收环形缓冲）---- */
void microros_uart_rxcplt_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* MICRO_ROS_TRANSPORT_H */
