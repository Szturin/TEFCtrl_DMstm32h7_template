/**
 * @file micro_ros_task.c
 * @brief micro-ROS 应用线程（RT-Thread）
 *
 * 功能示例：
 *   Publisher  → /stm32/motor_rpm   (std_msgs/Float32MultiArray)
 *   Subscriber ← /stm32/cmd_vel    (geometry_msgs/Twist)
 *   Timer      → 每 10ms 发布一次电机转速
 *
 * 编译依赖：libmicroros.a + microros_include/（Docker 构建后生成）
 *
 * 与 micro-ROS Agent 连接方式：
 *   PC 端运行：
 *     ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 460800
 */

#include "micro_ros_task.h"

/* 仅在 libmicroros.a 存在时编译此文件 */
#ifdef MICRO_ROS_ENABLED

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>
#include <geometry_msgs/msg/twist.h>
#include <rmw_microros/rmw_microros.h>

#include "rtthread.h"
#include "micro_ros_transport.h"
#include "micro_ros_allocators.h"  /* microros_allocate 等 */

/* ------------------------------------------------------------------ */
/* 宏：错误检查（micro-ROS 返回 RCL_RET_OK 为成功）                     */
/* ------------------------------------------------------------------ */
#define RCCHECK(fn) \
    do { \
        rcl_ret_t rc = (fn); \
        if (rc != RCL_RET_OK) { \
            rt_kprintf("[mros] ERROR %s:%d code=%d\n", __FILE__, __LINE__, (int)rc); \
            return; \
        } \
    } while (0)

#define RCSOFTCHECK(fn) \
    do { \
        rcl_ret_t rc = (fn); \
        if (rc != RCL_RET_OK) { \
            rt_kprintf("[mros] WARN  %s:%d code=%d\n", __FILE__, __LINE__, (int)rc); \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/* 消息对象                                                             */
/* ------------------------------------------------------------------ */
static std_msgs__msg__Float32MultiArray  motor_rpm_msg;
static geometry_msgs__msg__Twist         cmd_vel_msg;

/* ------------------------------------------------------------------ */
/* 定时器回调：每 10ms 发布电机转速                                      */
/* ------------------------------------------------------------------ */
static rcl_publisher_t  motor_rpm_pub;

static void publish_timer_cb(rcl_timer_t *timer, int64_t last_call_time)
{
    (void)last_call_time;
    if (timer == NULL) return;

    /* TODO: 从实际电机驱动获取转速数据 */
    static float rpm_data[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    motor_rpm_msg.data.data     = rpm_data;
    motor_rpm_msg.data.size     = 4;
    motor_rpm_msg.data.capacity = 4;

    RCSOFTCHECK(rcl_publish(&motor_rpm_pub, &motor_rpm_msg, NULL));
}

/* ------------------------------------------------------------------ */
/* 订阅回调：接收速度指令                                               */
/* ------------------------------------------------------------------ */
static void cmd_vel_cb(const void *msgin)
{
    const geometry_msgs__msg__Twist *msg =
        (const geometry_msgs__msg__Twist *)msgin;

    /* TODO: 将速度指令传给底盘控制 */
    float vx = (float)msg->linear.x;
    float vy = (float)msg->linear.y;
    float wz = (float)msg->angular.z;

    rt_kprintf("[mros] cmd_vel: vx=%.2f vy=%.2f wz=%.2f\n", vx, vy, wz);
    (void)vx; (void)vy; (void)wz;
}

/* ------------------------------------------------------------------ */
/* micro-ROS 线程主体                                                   */
/* ------------------------------------------------------------------ */
void micro_ros_thread_entry(void *param)
{
    (void)param;

    /* 1. 注册 RT-Thread 内存分配器 */
    rcl_allocator_t allocator = {
        .allocate      = microros_allocate,
        .deallocate    = microros_deallocate,
        .reallocate    = microros_reallocate,
        .zero_allocate = microros_zero_allocate,
        .state         = NULL,
    };

    /* 2. 注册自定义 UART Transport */
    rmw_uros_set_custom_transport(
        true,   /* framing enabled */
        NULL,
        microros_transport_open,
        microros_transport_close,
        microros_transport_write,
        microros_transport_read);

    /* 3. 等待 Agent 连接（无限重试，每次间隔 500ms） */
    rt_kprintf("[mros] Waiting for micro-ROS Agent...\n");
    while (rmw_uros_ping_agent(500, 3) != RMW_RET_OK)
    {
        rt_thread_mdelay(500);
    }
    rt_kprintf("[mros] Agent connected!\n");

    /* 4. 初始化 support（含 DDS 上下文） */
    rclc_support_t support;
    RCCHECK(rclc_support_init_with_options(
        &support, 0, NULL,
        &(rcl_init_options_t){0},
        &allocator));

    /* 5. 创建节点 */
    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "stm32h7_node", "", &support));

    /* 6. 创建 Publisher（电机转速） */
    RCCHECK(rclc_publisher_init_default(
        &motor_rpm_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
        "stm32/motor_rpm"));

    /* 7. 创建 Subscriber（速度指令） */
    rcl_subscription_t cmd_vel_sub;
    RCCHECK(rclc_subscription_init_default(
        &cmd_vel_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "stm32/cmd_vel"));

    /* 8. 创建定时器（10ms 周期发布） */
    rcl_timer_t timer;
    RCCHECK(rclc_timer_init_default(
        &timer, &support,
        RCL_MS_TO_NS(10),
        publish_timer_cb));

    /* 9. 创建执行器（1 subscriber + 1 timer = 2 handles） */
    rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
    RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
    RCCHECK(rclc_executor_add_subscription(
        &executor, &cmd_vel_sub, &cmd_vel_msg, cmd_vel_cb, ON_NEW_DATA));
    RCCHECK(rclc_executor_add_timer(&executor, &timer));

    rt_kprintf("[mros] Node running. Topics: /stm32/motor_rpm, /stm32/cmd_vel\n");

    /* 10. 主循环：spin */
    while (1)
    {
        RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));
        rt_thread_mdelay(1);
    }

    /* 清理（正常不会到这里） */
    RCSOFTCHECK(rcl_subscription_fini(&cmd_vel_sub, &node));
    RCSOFTCHECK(rcl_publisher_fini(&motor_rpm_pub, &node));
    RCSOFTCHECK(rcl_node_fini(&node));
    rclc_support_fini(&support);
}

#else  /* MICRO_ROS_ENABLED 未定义时，提供空实现 */

#include "rtthread.h"

void micro_ros_thread_entry(void *param)
{
    (void)param;
    rt_kprintf("[mros] micro-ROS not enabled. Build libmicroros.a first.\n");
    rt_kprintf("[mros] Run: ./cmake/build_microros.sh (requires Docker)\n");
    /* 线程结束 */
}

#endif /* MICRO_ROS_ENABLED */
