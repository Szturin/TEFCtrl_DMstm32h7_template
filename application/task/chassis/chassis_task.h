#ifndef CHASSIS_TASK_H
#define CHASSIS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* 底盘工作模式 */
typedef enum {
    CHASSIS_ZERO_FORCE,         // 零力（电机不输出）
    CHASSIS_FREE,               // 自由模式（无旋转）
    CHASSIS_FOLLOW_GIMBAL_YAW,  // 跟随云台 Yaw
    CHASSIS_ROTATE,             // 小陀螺
} Chassis_Mode_e;

void chassis_task_init(void);
void chassis_task_proc(void);

// USB/VOFA+ 命令解析（调试用）
void chassis_cmd_parse(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_TASK_H
