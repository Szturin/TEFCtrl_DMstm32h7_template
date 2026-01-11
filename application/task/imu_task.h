#ifndef __IMU_TASK_H__
#define __IMU_TASK_H__
#ifdef __cplusplus
extern "C" {
#endif
#define INS_YAW_ADDRESS_OFFSET    0
#define INS_PITCH_ADDRESS_OFFSET  1
#define INS_ROLL_ADDRESS_OFFSET   2

void ImuTask_Init();

void ImuTask_Entry(void);

#ifdef __cplusplus
}
#endif
#endif
