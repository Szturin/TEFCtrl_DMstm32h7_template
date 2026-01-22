/**
 * @file motor_def.h
 * @author neozng
 * @brief  ���ͨ�õ����ݽṹ����
 * @version beta
 * @date 2022-11-01
 *
 * @copyright Copyright (c) 2022 HNU YueLu EC all rights reserved
 *
 */

#ifndef MOTOR_DEF_H
#define MOTOR_DEF_H

#include "can/bsp_fdcan.h"

typedef enum
{
    MOTOR_STOP = 0,
    MOTOR_ENALBED = 1,
} Motor_Working_Type_e;

/* �������ö�� */
typedef enum
{
    MOTOR_TYPE_NONE = 0,
    GM6020,
    M3508,
    M2006,
    LK9025,
    HT04,
} Motor_Type_e;

/* ���ڳ�ʼ��CAN����Ľṹ��,������ͨ�� */
typedef struct
{
    Motor_Type_e motor_type;
    FDCAN_Init_Config_s can_init_config;
} Motor_Init_Config_s;

#endif // !MOTOR_DEF_H
