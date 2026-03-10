#ifndef REFEREE_H
#define REFEREE_H

#include "rm_referee.h"

typedef struct{
    uint8_t refresh_flag;
    float pitch;
}ui_event_e;

/**
 * @brief 初始化裁判系统交互任务(UI和多机通信)
 *
 */
referee_info_t *UITaskInit(UART_HandleTypeDef *referee_usart_handle, Referee_Interactive_info_t *UI_data);

/**
 * @brief 在referee task之前调用,添加在freertos.c中
 * 
 */
void MyUIInit();

/**
 * @brief 裁判系统交互任务(UI和多机通信)
 *
 */
void UITask();
void UI_event_callback();
Referee_Interactive_info_t *UI_CMD();
#endif // REFEREE_H
