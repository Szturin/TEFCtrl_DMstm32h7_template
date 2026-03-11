#ifndef DAEMON_H
#define DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "string.h"

#define DAEMON_MX_CNT 64

/* 模块离线处理函数指针 */
typedef void (*offline_callback)(void *);

/* daemon结构体定义 */
typedef struct daemon_ins
{
    uint16_t reload_count;     // 重载值（喂狗周期，单位：ms）
    offline_callback callback; // 掉线回调，边沿触发

    uint16_t temp_count;       // 当前倒计数，减为零说明模块离线
    uint8_t  is_online;        // 在线标志：1=在线，0=离线
    void    *owner_id;         // 拥有者指针
} DaemonInstance;

/* daemon初始化配置 */
typedef struct
{
    uint16_t reload_count;     // 超时阈值（ms）
    uint16_t init_count;       // 首次上线宽限时间（ms），0 则使用 reload_count
    offline_callback callback; // 掉线回调（边沿触发，掉线瞬间调用一次）
    void *owner_id;
} Daemon_Init_Config_s;

/**
 * @brief 注册一个 daemon 实例（静态内存池）
 */
DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config);

/**
 * @brief 喂狗：模块收到新数据时调用
 */
void DaemonReload(DaemonInstance *instance);

/**
 * @brief 查询模块是否在线
 */
uint8_t DaemonIsOnline(DaemonInstance *instance);

/**
 * @brief 启动软件看门狗定时器（RT-Thread 软件定时器，1ms 周期）
 *        在 rtt_app_threads_init() 中调用一次即可，无需再手动调用 DaemonTask。
 */
void DaemonStart(void);

#ifdef __cplusplus
}
#endif

#endif // !DAEMON_H
