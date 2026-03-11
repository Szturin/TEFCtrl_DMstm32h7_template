#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "bsp/can/bsp_fdcan.h"

void motor_task_init(void);
void motor_task_proc(void);
void shoot_task_init(void);
void shoot_task_proc(void);
void unitree_task_init(void);
void unitree_task_proc(void);
void Robot_Init(void);
void Chassis_Task(void);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_TASK_H
