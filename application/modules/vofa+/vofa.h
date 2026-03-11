#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define VOFA_MAX_CH 10

// 发送 JustFloat 数据帧（非阻塞，USB 忙时跳过）
void vofa_justfloat_send(float *data, uint8_t ch_count);

// 初始化 VOFA+ 系统（注册 USB 接收回调）
void vofa_init(void);

// 注册调参回调，收到 "Kp:1.5\n" 类命令时被调用
// 格式: handler(command_string)，command 已去掉末尾 \r\n
typedef void (*vofa_cmd_handler_t)(const char *cmd);
void vofa_register_cmd_handler(vofa_cmd_handler_t handler);

#ifdef __cplusplus
}
#endif
