#include "daemon.h"
#include <stdlib.h>
#include <string.h>

// ���ڱ������е�daemon instance
static DaemonInstance *daemon_instances[DAEMON_MX_CNT] = {NULL};
static uint8_t idx; // ���ڼ�¼��ǰ��daemon instance����,��ϻص�ʹ��

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{
    DaemonInstance *instance = (DaemonInstance *)malloc(sizeof(DaemonInstance));
    memset(instance, 0, sizeof(DaemonInstance));

    instance->owner_id = config->owner_id;
    instance->reload_count = config->reload_count == 0 ? 100 : config->reload_count; // Ĭ��ֵΪ100
    instance->callback = config->callback;
    instance->temp_count = config->init_count == 0 ? 100 : config->init_count; // Ĭ��ֵΪ100,��ʼ����

    instance->temp_count = config->reload_count;
    daemon_instances[idx++] = instance;
    return instance;
}

/* "ι��"���� */
void DaemonReload(DaemonInstance *instance)
{
    instance->temp_count = instance->reload_count;
}

uint8_t DaemonIsOnline(DaemonInstance *instance)
{
    return instance->temp_count > 0;
}

/*�������Ź�������ĳ�STM32Ӳ�����Ź��ж�*/
void DaemonTask()
{
    DaemonInstance *dins; // ��߿ɶ���ͬʱ���ͷô濪��
    for (size_t i = 0; i < idx; ++i)
    {

        dins = daemon_instances[i];
        if (dins->temp_count > 0) // �������������ֵ,˵����һ��ι����û�г�ʱ,���������һ
            dins->temp_count--;
        else if (dins->callback) // ������˵����ʱ��,���ûص�����(����еĻ�)
        {
            dins->callback(dins->owner_id); // module�ڿ��Խ�owner_idǿ������ת�����������ʹӶ������ض�module��offline callback
            // @todo Ϊ������/led���������߱����Ĺ���,�ǳ��ؼ�!
        }
    }
}