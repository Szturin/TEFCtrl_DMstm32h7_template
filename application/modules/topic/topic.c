#include "topic.h"

/* 懒初始化：第一次使用时初始化 mutex（单核无竞争风险） */
static void ensure_init(topic_t *t)
{
    if (!t->inited) {
        rt_enter_critical();
        if (!t->inited) {
            rt_mutex_init(&t->lock, "topic", RT_IPC_FLAG_PRIO);
            t->inited = 1;
        }
        rt_exit_critical();
    }
}

void topic_publish(topic_t *t, const void *data)
{
    ensure_init(t);

    rt_mutex_take(&t->lock, RT_WAITING_FOREVER);
    memcpy(t->buf, data, t->size);
    rt_mutex_release(&t->lock);

    /* 广播：遍历链表通知所有订阅者 */
    for (topic_sub_t *s = t->subs; s; s = s->next)
        rt_sem_release(&s->sem);
}

void topic_peek(topic_t *t, void *out)
{
    ensure_init(t);
    rt_mutex_take(&t->lock, RT_WAITING_FOREVER);
    memcpy(out, t->buf, t->size);
    rt_mutex_release(&t->lock);
}

void topic_sub_init(topic_sub_t *sub, topic_t *t, const char *name)
{
    ensure_init(t);
    rt_sem_init(&sub->sem, name, 0, RT_IPC_FLAG_FIFO);

    /* 头插法加入链表 */
    sub->topic = t;
    rt_mutex_take(&t->lock, RT_WAITING_FOREVER);
    sub->next = t->subs;
    t->subs   = sub;
    rt_mutex_release(&t->lock);
}

rt_err_t topic_sub_wait(topic_sub_t *sub, void *out, rt_int32_t timeout_ms)
{
    rt_err_t ret = rt_sem_take(&sub->sem, (rt_tick_t)timeout_ms);
    if (ret == RT_EOK)
        topic_peek(sub->topic, out);
    return ret;
}
