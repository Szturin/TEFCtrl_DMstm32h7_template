#include "pid_tune.h"

/**
 * @note VOFA+ 串口滑块调参接口
 *       发送格式：  "Kp:1.5\r\n"  "Ki:0.1\r\n"  "Kd:0.05\r\n"
 * @todo 后续可接入 Flash 存储，实现断电参数保存
 */

PID_Params_t PID_Test;

void PID_UpdateFromCommand(PID_Params_t *pid, const char *command)
{
    if      (strncmp(command, "Kp:", 3) == 0) pid->Kp = atof(command + 3);
    else if (strncmp(command, "Ki:", 3) == 0) pid->Ki = atof(command + 3);
    else if (strncmp(command, "Kd:", 3) == 0) pid->Kd = atof(command + 3);
    else if (strncmp(command, "Ks:", 3) == 0) pid->Ks = atof(command + 3);
}

void PID_PrintParams(UART_HandleTypeDef *huart, PID_Params_t *pid)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Kp:%.2f Ki:%.2f Kd:%.2f\r\n", pid->Kp, pid->Ki, pid->Kd);
    HAL_UART_Transmit(huart, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
}
