#ifndef DJI_MOTOR_H
#define DJI_MOTOR_H

#include "can/bsp_fdcan.h" // 使用此前定义的FDCAN底层
#include "motor_def.h"

#define DJI_MOTOR_CNT 12

/* 物理常数与滤波系数 */
#define SPEED_SMOOTH_COEF 0.95f
#define CURRENT_SMOOTH_COEF 0.9f
#define ECD_ANGLE_COEF_DJI 0.043945f // (360/8192)
#define TORQUE_COEF 0.00036621f      // 电流转转矩系数
#ifdef __cplusplus
extern "C" { // 存放C接口
#endif
typedef struct {
    uint16_t last_ecd;
    uint16_t ecd;
    float angle_single_round;
    int16_t speed_rpm;
    int16_t real_current;
    uint8_t temperature;
    float total_angle;
    int32_t total_round;
} DJI_Motor_Measure_s;

typedef struct {
    DJI_Motor_Measure_s measure;
    int16_t output_set;
    float power_output;

    FDCANInstance *motor_can_instance; // 换成FDCAN实例
    uint8_t sender_group;              // 发送组索引
    uint8_t message_num;               // 组内编号 (0~3)

    Motor_Type_e motor_type;
    uint8_t stop_flag;
} DJIMotorInstance;

/* 接口函数 */
DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *config);
void DJIMotorSetRef_Current(DJIMotorInstance *motor, int16_t current);
void DJIMotorControl(void);
void DJIMotorStop(DJIMotorInstance *motor);
void DJIMotorEnable(DJIMotorInstance *motor);
#ifdef __cplusplus
}
#endif
void DJIMotorControl(void);
#endif