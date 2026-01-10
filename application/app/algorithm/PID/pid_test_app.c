#include "pid_test_app.h"

/**
 * @note PID上位机快速调参，调用此源文件中的API可以配合VOFA+上位机快速调参
 * @todo 使用flash使得PID参数能够断电保留
 * */

/*全局变量 warning ！！！*/
PID_Params_t PID_Test;

// 初始化PID参数
//void PID_Init(PID_Params_t *pid, float kp, float ki, float kd) {
//    pid->Kp = kp;
//    pid->Ki = ki;
//    pid->Kd = kd;
//}

// 从上位机命令解析并更新PID参数
void PID_UpdateFromCommand(PID_Params_t *pid, const char *command) {
    if (strncmp(command, "Kp:", 3) == 0) {
        pid->Kp = atof(command + 3);
    } else if (strncmp(command, "Ki:", 3) == 0) {
        pid->Ki = atof(command + 3);
    } else if (strncmp(command, "Kd:", 3) == 0) {
        pid->Kd = atof(command + 3);
    } else if (strncmp(command, "Ks:", 3) == 0) {
        pid->Ks = atof(command + 3);
    }
}

// 串口打印PID参数
void PID_PrintParams(UART_HandleTypeDef *huart, PID_Params_t *pid) {
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Kp: %.2f, Ki: %.2f, Kd: %.2f\r\n",
             pid->Kp, pid->Ki, pid->Kd);
    HAL_UART_Transmit(huart, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
}
