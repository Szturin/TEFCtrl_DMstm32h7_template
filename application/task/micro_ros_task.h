#ifndef MICRO_ROS_TASK_H
#define MICRO_ROS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief micro-ROS 线程入口（由 RT-Thread 线程调用）
 *        包含：节点创建 → publisher/subscriber → executor 循环
 */
void micro_ros_thread_entry(void *param);

#ifdef __cplusplus
}
#endif

#endif /* MICRO_ROS_TASK_H */
