#include "bsp_system.h"
#include "remoter_uart.h"
#include "cmd_parse/uart_task.h"

/*全局变量 warning ！！！*/
wbus_rc_info_t wbus_rc;

void remote_filter(int16_t *data, int16_t new_data, float filter_a){
    *data = (int16_t)((float)new_data * filter_a + (1- filter_a) * (float)(*data));
}

void parse_wbus_data(wbus_rc_info_t *rc_wbus, uint8_t *buff)
{
    int16_t remote_buffer[4];

    if (buff[0] != 0x0F)
    {
        return; // 检查起始位
    }

    /*遥感通道缓存*/
    remote_buffer[0] = ((buff[1] | (buff[2] << 8)) & 0x07FF);
    remote_buffer[0] -= 1024;
    remote_buffer[1] = ((buff[2] >> 3) | (buff[3] << 5)) & 0x07FF ;
    remote_buffer[1] -= 1024;
    remote_buffer[2] = ((buff[3] >> 6) | (buff[4] << 2) | (buff[5] << 10)) & 0x07FF;
    remote_buffer[2] -= 1024;
    remote_buffer[3] = ((buff[5] >> 1) | (buff[6] << 7)) & 0x07FF;
    remote_buffer[3] -= 1024;

    /*左右遥杆，进行一阶低通滤波*/
    remote_filter(&rc_wbus->remote.ch1,remote_buffer[0],0.38f);
    remote_filter(&rc_wbus->remote.ch2,remote_buffer[1],0.38f);
    remote_filter(&rc_wbus->remote.ch3,remote_buffer[2],0.38f);
    remote_filter(&rc_wbus->remote.ch4,remote_buffer[3],0.38f);

    /*拨片通道*/
    rc_wbus->remote.SA = (((buff[6] >> 4) | (buff[7] << 4)) & 0x07FF) / 671 - 1;				            // 拨片通道1
    rc_wbus->remote.SB = (((buff[7] >> 7) | (buff[8] << 1) | (buff[9] << 9)) & 0x07FF) / 671 - 1;         // 拨片通道2
    rc_wbus->remote.SC = ((buff[9] >> 2) | (buff[10]<<6)) & 0x07FF / 671 - 1;                           // 拨片通道3
    rc_wbus->remote.SD =  (((buff[10] >> 5) | (buff[11] << 3)) & 0x07FF) / 671 - 1;                       // 拨片通道4
    rc_wbus->remote.SE = ((buff[12] | (buff[13] << 8)) & 0x07FF) / 671 - 1;                             // 拨片通道5
    rc_wbus->remote.SF = ((buff[13] >> 3) | (buff[14] << 5)) & 0x07FF / 671 - 1;                        // 拨片通道6
    rc_wbus->remote.SG =  ((buff[14] >> 6) | (buff[15] << 2) | (buff[16] << 10)) & 0x07FF / 671 - 1;    // 拨片通道7
    rc_wbus->remote.SH =  ((buff[16] >> 1) | (buff[17] << 7)) & 0x07FF / 671 - 1;                       // 拨片通道8

    /*旋钮通道*/
    rc_wbus->remote.LD = ((buff[17] >> 4) | (buff[18] << 4)) & 0x07FF ;				         // 旋钮1
    rc_wbus->remote.LD -= 1024;
    rc_wbus->remote.RD= ((buff[18] >> 7) | (buff[19] << 1) | (buff[20] << 9)) & 0x07FF;      // 旋钮2
    rc_wbus->remote.RD -= 1024;

    /*拨轮通道*/
    rc_wbus->remote.LS = ((buff[20] >> 2) | (buff[21]<<6)) & 0x07FF;                         // 拨轮通道1
    rc_wbus->remote.LS -= 1024;
    rc_wbus->remote.RS= ((buff[21] >> 5) | (buff[22] << 3)) & 0x07FF;                        // 拨轮通道2
    rc_wbus->remote.RS -= 1024;
}


// UART 初始化 (DMA + 空闲中断)
wbus_rc_info_t* RemoteInit(void)
{
    // 配置 HAL 的 DMA + 空闲中断模式
   // HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_rx_dma_buffer, sizeof(uart1_rx_dma_buffer));
    // 禁用 DMA 半传输中断 (Half Transfer IT)，减少不必要的中断
   // __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

    return &wbus_rc;
}