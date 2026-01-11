#ifndef IMU_TEMP_CTRL_H
#define IMU_TEMP_CTRL_H
#include "main.h"
#include "BMI088/BMI088driver.h"
#include "gpio.h"
#include "tim.h"
#include "adc.h"
#include "algorithm/_imu/algorithm.h"
#include "algorithm/_imu/kalman_filter.h"
#include "algorithm/_imu/QuaternionEKF.h"
#include "imu_temp_ctrl.h"
#include "algorithm/_imu/pid.h"
#include "algorithm/Mahony/MahonyAHRS.h"
#ifdef __cplusplus
extern "C" {
#endif

void INS_Init(void);

void IMU_Task(void);

#ifdef __cplusplus
}
#endif
#endif // IMU_TEMP_CTRL_H
