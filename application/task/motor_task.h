#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "app/motor/dm_motor/dm_motor_ctrl.h"
#include "app/motor/dm_motor/dm_motor_drv.h"
#include "bsp/can/bsp_fdcan.h"

void motor_task_init(void);
void motor_task_proc(void);
void shoot_task_init(void);
void shoot_task_proc(void);
#ifdef __cplusplus
}
#endif

#endif // MOTOR_TASK_H
