#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "main.h"

#ifdef __cplusplus
#include <cstdint>
#include <cstring>
#else
#include <stdint.h>
#include <string.h>
#endif

#ifdef __cplusplus

/**
 * @brief 任务类 - 封装任务的配置和状态（内部使用）
 */
class Task {
public:
    // 默认构造函数（用于数组初始化）
    Task();

    // 带参数的构造函数
    Task(void (*task_func)(void), uint32_t period_ms, uint8_t priority = 0, bool enabled = true);

    void execute();
    bool isReadyToRun(uint32_t current_time) const;
    void updateLastTime(uint32_t time) { last_time_ = time; }

    uint32_t getPeriod() const { return period_ms_; }
    void setPeriod(uint32_t period_ms) { period_ms_ = period_ms; }
    uint8_t getPriority() const { return priority_; }
    void setPriority(uint8_t priority) { priority_ = priority; }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

private:
    void (*task_func_)(void);
    uint32_t period_ms_;
    uint32_t last_time_;
    uint8_t priority_;
    bool enabled_;
};

/**
 * @brief 调度器类 - 面向对象的裸机任务调度器
 */
class Scheduler {
public:
    /**
     * @brief 构造函数
     */
    Scheduler();

    /**
     * @brief 初始化调度器（必须在添加任务前调用）
     */
    void init();

    /**
     * @brief 添加任务到调度器
     * @param task_func 任务函数指针
     * @param period_ms 任务执行周期（毫秒）
     * @param priority 任务优先级（数值越小优先级越高，默认为 0）
     * @return true 添加成功，false 任务列表已满
     */
    bool addTask(void (*task_func)(void), uint32_t period_ms, uint8_t priority = 0);

    /**
     * @brief 运行调度器（在主循环中调用）
     */
    void run();

    /**
     * @brief 获取当前任务数量
     */
    uint8_t getTaskCount() const { return task_count_; }

    /**
     * @brief 清空所有任务
     */
    void clearAllTasks();

    /**
     * @brief 启用/禁用指定索引的任务
     */
    void enableTask(uint8_t index);
    void disableTask(uint8_t index);

    /**
     * @brief 修改指定索引任务的周期
     */
    void setTaskPeriod(uint8_t index, uint32_t period_ms);

private:
    static constexpr uint8_t MAX_TASKS = 16;  // 最大任务数量
    Task tasks_[MAX_TASKS];                   // 任务对象数组（静态分配）
    bool task_used_[MAX_TASKS];               // 任务槽位使用标记
    uint8_t task_count_;                      // 当前任务数量
    uint32_t last_tick_;                      // 上次调度时间（用于优化）
};

#endif // __cplusplus

// ==================== C 语言接口（用于 C/C++ 混编）====================
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C 语言调度器句柄（不透明指针）
 */
typedef void* SchedulerHandle;

/**
 * @brief 创建调度器实例
 * @return 调度器句柄
 */
SchedulerHandle scheduler_create(void);

/**
 * @brief 初始化调度器
 * @param handle 调度器句柄
 */
void scheduler_init(SchedulerHandle handle);

/**
 * @brief 添加任务
 * @param handle 调度器句柄
 * @param task_func 任务函数指针
 * @param period_ms 任务周期（毫秒）
 * @param priority 任务优先级（0 = 最高）
 * @return 1 成功，0 失败
 */
int scheduler_add_task(SchedulerHandle handle, void (*task_func)(void), uint32_t period_ms, uint8_t priority);

/**
 * @brief 运行调度器
 * @param handle 调度器句柄
 */
void scheduler_run(SchedulerHandle handle);

/**
 * @brief 获取任务数量
 * @param handle 调度器句柄
 * @return 任务数量
 */
uint8_t scheduler_get_task_count(SchedulerHandle handle);

/**
 * @brief 启用任务
 * @param handle 调度器句柄
 * @param index 任务索引
 */
void scheduler_enable_task(SchedulerHandle handle, uint8_t index);

/**
 * @brief 禁用任务
 * @param handle 调度器句柄
 * @param index 任务索引
 */
void scheduler_disable_task(SchedulerHandle handle, uint8_t index);

/**
 * @brief 设置任务周期
 * @param handle 调度器句柄
 * @param index 任务索引
 * @param period_ms 新的周期（毫秒）
 */
void scheduler_set_task_period(SchedulerHandle handle, uint8_t index, uint32_t period_ms);

/**
 * @brief 销毁调度器实例
 * @param handle 调度器句柄
 */
void scheduler_destroy(SchedulerHandle handle);

#ifdef __cplusplus
}
#endif

#endif // SCHEDULER_H
