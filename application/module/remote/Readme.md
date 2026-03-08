## CubeMX配置

- 串口属性
  - 波特率100000Bits/s，==长度9bit==，偶校验，1位停止位。

![image-20250219213640770](S:\6_rt_thread_project\robomaster\TEF_Control_Base_new\applications\RM_APP\remote\Readme.assets\image-20250219213640770.png)

- DMA

  ![image-20250219214318352](S:\6_rt_thread_project\robomaster\TEF_Control_Base_new\applications\RM_APP\remote\Readme.assets\image-20250219214318352.png)

## 使用示例

```c
#include "uart_cmd_task.h"

uint8_t uart1_rx_dma_buffer[WBUS_BUFLEN]={0};//串口2DMA缓冲区
uint8_t uart1_read_buffer[WBUS_BUFLEN];//定义环形缓存区数组
static ringbuffer_t uart1_rb; //定义ringbuffer_t类型结构体变量

void uart_ringbuffer_init(void){
    ringbuffer_init(&uart1_rb);//遥控器串口
}

//空闲中断回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART1){// 遥控器
        if(!ringbuffer_is_full(&uart1_rb)){
            ringbuffer_write(&uart1_rb,uart1_rx_dma_buffer,Size);
        }

        /*重新开启DMA接收*/
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_rx_dma_buffer, sizeof(uart1_rx_dma_buffer));
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

        memset(uart1_rx_dma_buffer,0,sizeof(uart1_rx_dma_buffer));
    }
}


void uart_proc(void)
{
    if( !ringbuffer_is_empty(&uart1_rb)){//遥控器

        ringbuffer_read(&uart1_rb, uart1_read_buffer, uart1_rb.itemCount);
        parse_wbus_data(&wbus_rc, uart1_read_buffer);
    }
}
```

