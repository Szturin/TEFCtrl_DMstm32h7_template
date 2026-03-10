#include "daemon.h"
#include "rtthread.h"
#include <string.h>

/* 静态内存池，避免 heap malloc 不确定性 */
static DaemonInstance daemon_pool[DAEMON_MX_CNT];
static uint8_t idx;

/* RT-Thread 软件定时器（静态分配） */
static struct rt_timer daemon_timer;

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{
    DaemonInstance *instance = &daemon_pool[idx++];
    memset(instance, 0, sizeof(DaemonInstance));

    instance->owner_id     = config->owner_id;
    instance->reload_count = config->reload_count ? config->reload_count : 100;
    instance->callback     = config->callback;
    instance->temp_count   = config->init_count ? config->init_count : instance->reload_count;
    instance->is_online    = 0;

    return instance;
}

void DaemonReload(DaemonInstance *instance)
{
    instance->temp_count = instance->reload_count;
    instance->is_online  = 1;
}

uint8_t DaemonIsOnline(DaemonInstance *instance)
{
    return instance->is_online;
}

/* 软件看门狗主逻辑：每 1ms 调用一次 */
static void daemon_tick(void *param)
{
    (void)param;
    for (uint8_t i = 0; i < idx; ++i)
    {
        DaemonInstance *d = &daemon_pool[i];
        if (d->temp_count > 0)
        {
            d->temp_count--;
        }
        else if (d->is_online)   /* 在线 → 离线：边沿触发，只执行一次 */
        {
            d->is_online = 0;
            if (d->callback)
                d->callback(d->owner_id);
        }
    }
}

/**
 * @brief 启动软件看门狗（RT-Thread 软件定时器，1ms 周期）
 *        在 rtt_app_threads_init() 中调用一次。
 */
void DaemonStart(void)
{
    rt_timer_init(&daemon_timer,
                  "daemon",
                  daemon_tick,
                  RT_NULL,
                  1,                                         /* 1 tick = 1ms */
                  RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(&daemon_timer);
}
