#ifndef MICRO_ROS_TASK_H
#define MICRO_ROS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file micro_ros_task.h
 * @brief micro-ROS 线程接口
 *
 * 功能：
 * - ROS 节点创建与初始化
 * - Publisher: 发布电机转速、IMU 数据等
 * - Subscriber: 订阅上位机速度指令
 * - 定时器回调：周期发布
 *
 * 编译开关：MICRO_ROS_ENABLED（在 rtconfig.h 中定义）
 * 依赖：libmicroros.a + microros_include/（Docker 构建）
 */

/**
 * @brief micro-ROS 线程入口函数
 *
 * @param param RT-Thread 线程参数（未使用）
 *
 * @note 由 RT-Thread 在 micro_ros 线程中调用
 *       负责：
 *       - 初始化 ROS 节点和通信
 *       - 创建 Publisher/Subscriber
 *       - 运行 executor 循环
 */
void micro_ros_thread_entry(void *param);

#ifdef __cplusplus
}
#endif

#endif /* MICRO_ROS_TASK_H */
