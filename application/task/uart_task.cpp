#include "uart_task.h"

static  ringbuffer_t rb_uart5;// 定义串口6的环形缓冲区
uint8_t uart5_rx_dma_buffer[WBUS_BUFLEN]={0};
static uint8_t uart5_read_buffer[WBUS_BUFLEN];
extern Uart_Instance uart_log;


void uart_ringbuffer_init(void){
    ringbuffer_init(&rb_uart5);// 图传串口

}

//空闲中断回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    //uart_log.printf("%d",wbus_rc.remote.ch1);
    if(huart->Instance == UART5){ // 图传串口

       // uart_log.printf("%s\r\n",uart5_rx_dma_buffer);
        ringbuffer_write(&rb_uart5, uart5_rx_dma_buffer,sizeof(uart5_rx_dma_buffer));
        memset(uart5_rx_dma_buffer,0,sizeof(uart5_rx_dma_buffer));
        HAL_UARTEx_ReceiveToIdle_DMA(&huart5, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));
        __HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_HT);
    }
}

/*串口处理任务*/
void UartTask(void)
{
    //uart_log.printf("%d",wbus_rc.remote.ch1);
    // @note : rt_ringbuffer_get 内部会自动计算缓冲区存入的数据长度，第三个参数表示最大存入数据长度
    // 如果缓冲区没有存入数据，此函数内部会return，避免无用的资源消耗
    // 处理串口1 数据（遥控器）
    if(ringbuffer_is_empty(&rb_uart5)){
        return;
    }
    // 从环形缓冲区读取数据到读取缓冲区
    ringbuffer_read(&rb_uart5, uart5_read_buffer, sizeof(uart5_read_buffer));


    uart_log.printf("%d %d %d %d\r\n",wbus_rc.remote.ch1,
                wbus_rc.remote.ch2,
            wbus_rc.remote.ch3,
    wbus_rc.remote.SH);

    parse_wbus_data(&wbus_rc, uart5_read_buffer);// 解析缓冲区数据到遥控器数据结构中
    memset(uart5_read_buffer, 0, sizeof(uart5_read_buffer));// 考虑数据溢出问题，后续


}

//----------------------------------------------------------------------------------------------------------------------//
/**
 * @brief 当串口发送/接收出现错误时,会调用此函数,此时这个函数要做的就是重新启动接收
 * @note  最常见的错误:奇偶校验/溢出/帧错误
 * @param huart 发生错误的串口
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == UART5){// 遥控器
        /*重新开启DMA接收*/
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart5_rx_dma_buffer, sizeof(uart5_rx_dma_buffer));// 配置 HAL 的 DMA + 空闲中断模式
        __HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_HT);// 禁用 DMA 半传输中断 (Half Transfer IT)，减少不必要的中断
    }
}