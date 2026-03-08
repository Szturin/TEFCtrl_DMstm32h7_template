#include "dji_motor.h"
#include <stdlib.h>
#include <string.h>

static uint8_t motor_idx = 0;
static DJIMotorInstance *dji_motor_instance[DJI_MOTOR_CNT] = {NULL};

/**
 * @brief FDCAN发送配置预设
 * DJI电机虽然挂在FDCAN总线上，但协议仍为经典CAN格式（8字节）
 */
static FDCANInstance sender_assignment[6]; // 2条总线 * 3个ID组
static uint8_t sender_enable_flag[6] = {0};

/* 内部初始化发送器：由于发送器不接收，仅作为发送容器 */
static void InitSenderAssignment(void) {
    uint16_t std_ids[3] = {0x1FF, 0x200, 0x2FF};
    for (int i = 0; i < 6; i++) {
        sender_assignment[i].fdcan_handle = (i < 3) ? &hfdcan1 : &hfdcan2;
        sender_assignment[i].txconf.Identifier = std_ids[i % 3];
        sender_assignment[i].txconf.IdType = FDCAN_STANDARD_ID;
        sender_assignment[i].txconf.TxFrameType = FDCAN_DATA_FRAME;
        sender_assignment[i].txconf.DataLength = FDCAN_DLC_BYTES_8;
        sender_assignment[i].txconf.BitRateSwitch = FDCAN_BRS_OFF; // DJI不支持高速数据段
        sender_assignment[i].txconf.FDFormat = FDCAN_CLASSIC_CAN;
    }
}

static void MotorSenderGrouping(DJIMotorInstance *motor, FDCAN_Init_Config_s *config) {
    uint8_t motor_id = config->tx_id - 1; // 外部传入的1~8
    uint8_t group_idx;

    if (motor->motor_type == M2006 || motor->motor_type == M3508) {
        if (motor_id < 4) {
            group_idx = (config->fdcan_handle == &hfdcan1) ? 1 : 4; // 0x200
            motor->message_num = motor_id;
        } else {
            group_idx = (config->fdcan_handle == &hfdcan1) ? 0 : 3; // 0x1FF
            motor->message_num = motor_id - 4;
        }
        config->rx_id = 0x200 + motor_id + 1;
    } else if (motor->motor_type == GM6020) {
        if (motor_id < 4) {
            group_idx = (config->fdcan_handle == &hfdcan1) ? 0 : 3; // 0x1FF
            motor->message_num = motor_id;
        } else {
            group_idx = (config->fdcan_handle == &hfdcan1) ? 2 : 5; // 0x2FF
            motor->message_num = motor_id - 4;
        }
        config->rx_id = 0x204 + motor_id + 1;
    }

    motor->sender_group = group_idx;
    sender_enable_flag[group_idx] = 1;
}

static void DecodeDJIMotor(FDCANInstance *_instance) {
    uint8_t *rxbuff = _instance->rx_buff;
    DJIMotorInstance *motor = (DJIMotorInstance *)_instance->id;
    DJI_Motor_Measure_s *m = &motor->measure;

    m->last_ecd = m->ecd;
    m->ecd = ((uint16_t)rxbuff[0]) << 8 | rxbuff[1];
    m->angle_single_round = ECD_ANGLE_COEF_DJI * (float)m->ecd;

    // 滤波处理
    m->speed_rpm = (int16_t)((1.0f - SPEED_SMOOTH_COEF) * (float)m->speed_rpm +
                             SPEED_SMOOTH_COEF * (float)((int16_t)(rxbuff[2] << 8 | rxbuff[3])));
    m->real_current = (int16_t)((1.0f - CURRENT_SMOOTH_COEF) * (float)m->real_current +
                                CURRENT_SMOOTH_COEF * (float)((int16_t)(rxbuff[4] << 8 | rxbuff[5])));
    m->temperature = rxbuff[6];

    // 多圈计算
    if (m->ecd - m->last_ecd > 4096) m->total_round--;
    else if (m->ecd - m->last_ecd < -4096) m->total_round++;
    m->total_angle = (float)m->total_round * 360.0f + m->angle_single_round;
}

DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *config) {
    static uint8_t sender_inited = 0;
    if (!sender_inited) {
        InitSenderAssignment();
        sender_inited = 1;
    }

    DJIMotorInstance *instance = (DJIMotorInstance *)malloc(sizeof(DJIMotorInstance));
    memset(instance, 0, sizeof(DJIMotorInstance));

    instance->motor_type = config->motor_type;

    // FDCAN 配置映射
    FDCAN_Init_Config_s fdcan_cfg = {
            .fdcan_handle = config->can_init_config.fdcan_handle, // 注意：此处需确保传入的是FDCAN句柄
            .tx_id = config->can_init_config.tx_id,
            .data_len = 8,
            .fdcan_module_callback = DecodeDJIMotor,
            .id = instance
    };

    MotorSenderGrouping(instance, &fdcan_cfg);
    instance->motor_can_instance = FDCANRegister(&fdcan_cfg); // 注册到FDCAN底层

    instance->stop_flag = 0;
    dji_motor_instance[motor_idx++] = instance;
    return instance;
}

void DJIMotorControl(void) {
    for (uint8_t i = 0; i < motor_idx; i++) {
        DJIMotorInstance *motor = dji_motor_instance[i];
        uint8_t group = motor->sender_group;
        uint8_t num = motor->message_num;
        int16_t set = (motor->stop_flag == 1) ? 0 : motor->output_set;

        // 填入缓冲：FDCANInstance自带tx_buff
        sender_assignment[group].tx_buff[2 * num] = (uint8_t)(set >> 8);
        sender_assignment[group].tx_buff[2 * num + 1] = (uint8_t)(set & 0xFF);
    }

    for (uint8_t i = 0; i < 6; i++) {
        if (sender_enable_flag[i]) {
            // 使用此前封装的FDCAN发送函数
            FDCANTransmit(&sender_assignment[i], 8);
        }
    }
}

/* 启停控制简化 */
void DJIMotorStop(DJIMotorInstance *motor) { motor->stop_flag = 1; }
void DJIMotorEnable(DJIMotorInstance *motor) { motor->stop_flag = 0; }
void DJIMotorSetRef_Current(DJIMotorInstance *motor, int16_t current) { motor->output_set = current; }