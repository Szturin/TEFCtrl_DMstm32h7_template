#ifndef DAEMON_H
#define DAEMON_H

#include "stdint.h"
#include "string.h"

#define DAEMON_MX_CNT 64

/* 模块离线处理函数指针 */
typedef void (*offline_callback)(void *);

/* daemon结构体定义 */
typedef struct daemon_ins
{
    uint16_t reload_count;     // 重载值（喂狗周期，单位：DaemonTask调用次数）
    offline_callback callback; // 掉线回调，边沿触发，仅在在线→离线瞬间调用一次

    uint16_t temp_count;       // 当前倒计数，减为零说明模块离线
    uint8_t  is_online;        // 在线标志：1=在线，0=离线；由DaemonReload置1
    void    *owner_id;         // 拥有者指针，注册时填入（如 DJI_Motor_Instance*）
} DaemonInstance;

/* daemon初始化配置 */
typedef struct
{
    uint16_t reload_count;     // 超时阈值，单位为DaemonTask调用次数
    uint16_t init_count;       // 上线等待时间（首次喂狗前的宽限倒计数，0则使用reload_count）
    offline_callback callback; // 掉线回调（边沿触发，掉线瞬间调用一次）

    void *owner_id;            // id取拥有daemon的实例地址，cast成void*
} Daemon_Init_Config_s;

/**
 * @brief 注册一个daemon实例（静态内存池，无malloc）
 */
DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config);

/**
 * @brief 喂狗：模块收到新数据时调用，重置超时计数并置为在线状态
 */
void DaemonReload(DaemonInstance *instance);

/**
 * @brief 查询模块是否在线
 * @return 1=在线，0=离线
 */
uint8_t DaemonIsOnline(DaemonInstance *instance);

/**
 * @brief 软件看门狗主任务，放入调度器定周期调用（推荐1ms）
 *        对每个实例递减计数；在线→离线瞬间边沿触发callback，此后不再重复
 */
void DaemonTask(void);

#endif // !DAEMON_H
