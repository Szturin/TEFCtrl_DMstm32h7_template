#ifndef PID_TEST_APP_H
#define PID_TEST_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_system.h"

// ����PID���ƽṹ��
typedef struct {
    float Kp;
    float Ki;
    float Kd;
	float Ks;
} PID_Params_t;

extern PID_Params_t PID_Test;

// ��ʼ��PID����
//void PID_Init(PID_Params_t *pid, float kp, float ki, float kd);

// ����PID����������λ�������н�����
void PID_UpdateFromCommand(PID_Params_t *pid, const char *command);

// ���ڴ�ӡPID����������ȷ����λ��������
void PID_PrintParams(UART_HandleTypeDef *huart, PID_Params_t *pid);

#ifdef __cplusplus
}
#endif

#endif 
