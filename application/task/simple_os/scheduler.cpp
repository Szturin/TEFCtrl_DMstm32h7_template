#include "scheduler.h"

// ==================== Task 类实现 ====================

/**
 * @brief 默认构造函数（用于数组初始化）
 */
Task::Task()
    : task_func_(nullptr),
      period_ms_(0),
      last_time_(0),
      priority_(0),
      enabled_(false) {
}

/**
 * @brief 带参数的构造函数
 */
Task::Task(void (*task_func)(void), uint32_t period_ms, uint8_t priority, bool enabled)
    : task_func_(task_func),
      period_ms_(period_ms),
      last_time_(0),
      priority_(priority),
      enabled_(enabled) {
}

void Task::execute() {
    if (task_func_ != nullptr && enabled_) {
        task_func_();
    }
}

bool Task::isReadyToRun(uint32_t current_time) const {
    if (!enabled_) {
        return false;
    }
    return (current_time >= last_time_ + period_ms_);
}

// ==================== Scheduler 类实现 ====================

/**
 * @brief 构造函数
 */
Scheduler::Scheduler() : task_count_(0), last_tick_(0) {
    // 初始化任务使用标记
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        task_used_[i] = false;
    }
}

/**
 * @brief 初始化调度器
 */
void Scheduler::init() {
    clearAllTasks();
    last_tick_ = 0;
}

/**
 * @brief 添加任务到调度器（静态分配 + 插入排序优化）
 */
bool Scheduler::addTask(void (*task_func)(void), uint32_t period_ms, uint8_t priority) {
    if (task_func == nullptr || task_count_ >= MAX_TASKS) {
        return false;
    }

    // 找到空闲槽位
    uint8_t free_slot = MAX_TASKS;
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (!task_used_[i]) {
            free_slot = i;
            break;
        }
    }

    if (free_slot >= MAX_TASKS) {
        return false;  // 没有空闲槽位
    }

    // 在空闲槽位就地构造 Task 对象（避免动态分配）
    tasks_[free_slot] = Task(task_func, period_ms, priority);
    task_used_[free_slot] = true;
    task_count_++;

    // 使用插入排序优化：找到插入位置并移动元素
    // 按优先级从小到大排序（优先级数值越小越优先）
    uint8_t insert_pos = 0;

    // 找到第一个优先级大于等于新任务的位置
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (task_used_[i] && i != free_slot) {
            if (tasks_[i].getPriority() > priority) {
                insert_pos = i;
                break;
            }
            insert_pos = i + 1;
        }
    }

    // 如果插入位置不是当前位置，需要调整数组
    if (insert_pos != free_slot) {
        Task temp = tasks_[free_slot];

        if (insert_pos < free_slot) {
            // 向前移动，后移中间元素
            for (uint8_t i = free_slot; i > insert_pos; i--) {
                tasks_[i] = tasks_[i - 1];
                task_used_[i] = task_used_[i - 1];
            }
        } else {
            // 向后移动，前移中间元素
            for (uint8_t i = free_slot; i < insert_pos - 1; i++) {
                tasks_[i] = tasks_[i + 1];
                task_used_[i] = task_used_[i + 1];
            }
            insert_pos--;
        }

        tasks_[insert_pos] = temp;
        task_used_[insert_pos] = true;
    }

    return true;
}

/**
 * @brief 运行调度器（优化版：缓存 + 快速路径）
 */
void Scheduler::run() {
    uint32_t current_time = HAL_GetTick();

    // 快速路径：如果时间没变化，直接返回（避免无效扫描）
    if (current_time == last_tick_) {
        return;
    }
    last_tick_ = current_time;

    // 遍历所有已使用的任务
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (!task_used_[i]) {
            continue;  // 跳过未使用的槽位
        }

        // 检查任务是否准备好运行
        if (tasks_[i].isReadyToRun(current_time)) {
            tasks_[i].execute();
            tasks_[i].updateLastTime(current_time);
        }
    }
}

/**
 * @brief 清空所有任务（无需释放内存）
 */
void Scheduler::clearAllTasks() {
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        task_used_[i] = false;
    }
    task_count_ = 0;
}

/**
 * @brief 启用指定索引的任务
 */
void Scheduler::enableTask(uint8_t index) {
    if (index < MAX_TASKS && task_used_[index]) {
        tasks_[index].enable();
    }
}

/**
 * @brief 禁用指定索引的任务
 */
void Scheduler::disableTask(uint8_t index) {
    if (index < MAX_TASKS && task_used_[index]) {
        tasks_[index].disable();
    }
}

/**
 * @brief 修改指定索引任务的周期
 */
void Scheduler::setTaskPeriod(uint8_t index, uint32_t period_ms) {
    if (index < MAX_TASKS && task_used_[index]) {
        tasks_[index].setPeriod(period_ms);
    }
}




// ==================== C 语言接口实现 ====================

extern "C" {

/**
 * @brief 创建调度器实例
 */
SchedulerHandle scheduler_create(void) {
    Scheduler* scheduler = new Scheduler();
    return static_cast<SchedulerHandle>(scheduler);
}

/**
 * @brief 初始化调度器
 */
void scheduler_init(SchedulerHandle handle) {
    if (handle != nullptr) {
        static_cast<Scheduler*>(handle)->init();
    }
}

/**
 * @brief 添加任务
 */
int scheduler_add_task(SchedulerHandle handle, void (*task_func)(void), uint32_t period_ms, uint8_t priority) {
    if (handle != nullptr) {
        return static_cast<Scheduler*>(handle)->addTask(task_func, period_ms, priority) ? 1 : 0;
    }
    return 0;
}

/**
 * @brief 运行调度器
 */
void scheduler_run(SchedulerHandle handle) {
    if (handle != nullptr) {
        static_cast<Scheduler*>(handle)->run();
    }
}

/**
 * @brief 获取任务数量
 */
uint8_t scheduler_get_task_count(SchedulerHandle handle) {
    if (handle != nullptr) {
        return static_cast<Scheduler*>(handle)->getTaskCount();
    }
    return 0;
}

/**
 * @brief 启用任务
 */
void scheduler_enable_task(SchedulerHandle handle, uint8_t index) {
    if (handle != nullptr) {
        static_cast<Scheduler*>(handle)->enableTask(index);
    }
}

/**
 * @brief 禁用任务
 */
void scheduler_disable_task(SchedulerHandle handle, uint8_t index) {
    if (handle != nullptr) {
        static_cast<Scheduler*>(handle)->disableTask(index);
    }
}

/**
 * @brief 设置任务周期
 */
void scheduler_set_task_period(SchedulerHandle handle, uint8_t index, uint32_t period_ms) {
    if (handle != nullptr) {
        static_cast<Scheduler*>(handle)->setTaskPeriod(index, period_ms);
    }
}

/**
 * @brief 销毁调度器实例
 */
void scheduler_destroy(SchedulerHandle handle) {
    if (handle != nullptr) {
        delete static_cast<Scheduler*>(handle);
    }
}

}  // extern "C"
