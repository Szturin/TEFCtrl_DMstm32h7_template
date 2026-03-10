#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "rtthread.h"
#include <string.h>

/**
 * @brief 轻量 topic 模块（发布/订阅，支持多订阅者）
 *
 * 用法：
 *   // topics.c  — 定义
 *   TOPIC_DEFINE(chassis_cmd, ChassisCmd_t);
 *
 *   // topics.h  — 声明
 *   TOPIC_EXTERN(chassis_cmd);
 *
 *   // 任意线程 — 发布
 *   topic_publish(&chassis_cmd, &cmd);
 *
 *   // 订阅线程 — 初始化时注册，然后阻塞等待
 *   static topic_sub_t sub;
 *   topic_sub_init(&sub, &chassis_cmd, "my_sub");
 *   topic_sub_wait(&sub, &cmd, 10);
 *
 *   // 轮询线程 — 直接读最新值（无需注册）
 *   topic_peek(&chassis_cmd, &cmd);
 */

typedef struct topic     topic_t;
typedef struct topic_sub topic_sub_t;

struct topic_sub {
    struct rt_semaphore  sem;
    topic_t             *topic;
    struct topic_sub    *next;   /* 侵入式链表 */
};

struct topic {
    void            *buf;
    rt_size_t        size;
    struct rt_mutex  lock;
    topic_sub_t     *subs;       /* 订阅者链表头 */
    rt_uint8_t       inited;
};

/* 定义 topic（放在 .c 文件） */
#define TOPIC_DEFINE(name, type)          \
    static type _##name##_storage;        \
    topic_t name = {                      \
        .buf    = &_##name##_storage,     \
        .size   = sizeof(type),           \
        .subs   = RT_NULL,                \
        .inited = 0,                      \
    }

/* 声明外部 topic（放在 .h 文件） */
#define TOPIC_EXTERN(name)  extern topic_t name

/* --- API --- */
void     topic_publish(topic_t *t, const void *data);
void     topic_peek(topic_t *t, void *out);
void     topic_sub_init(topic_sub_t *sub, topic_t *t, const char *name);
rt_err_t topic_sub_wait(topic_sub_t *sub, void *out, rt_int32_t timeout_ms);

#ifdef __cplusplus
}
#endif
