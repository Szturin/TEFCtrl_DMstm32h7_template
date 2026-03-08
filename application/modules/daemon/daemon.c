#include "daemon.h"
#include <string.h>

// 静态内存池，避免 heap malloc 在裸机/RTOS 中的不确定性
static DaemonInstance daemon_pool[DAEMON_MX_CNT];
static uint8_t idx; // 已注册数量

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{
    DaemonInstance *instance = &daemon_pool[idx++];
    memset(instance, 0, sizeof(DaemonInstance));

    instance->owner_id    = config->owner_id;
    instance->reload_count = config->reload_count ? config->reload_count : 100;
    instance->callback    = config->callback;
    // init_count：首次上线的宽限计数；0则使用 reload_count
    instance->temp_count  = config->init_count ? config->init_count : instance->reload_count;
    instance->is_online   = 0; // 未喂狗前不视为在线，callback不触发

    return instance;
}

/* 喂狗：模块收到数据时调用，重置计数并标记为在线 */
void DaemonReload(DaemonInstance *instance)
{
    instance->temp_count = instance->reload_count;
    instance->is_online  = 1;
}

uint8_t DaemonIsOnline(DaemonInstance *instance)
{
    return instance->is_online;
}

/* 软件看门狗主任务，推荐 1ms 调用一次 */
void DaemonTask(void)
{
    for (uint8_t i = 0; i < idx; ++i)
    {
        DaemonInstance *d = &daemon_pool[i];
        if (d->temp_count > 0)
        {
            d->temp_count--;
        }
        else if (d->is_online) // 在线→离线 边沿触发，只执行一次
        {
            d->is_online = 0;
            if (d->callback)
                d->callback(d->owner_id);
        }
        // is_online=0 且 temp_count=0：持续离线，不再重复触发
    }
}
