//
// Created by 123 on 2026/3/11.
//

#include "powermeter.h"
#include <stdlib.h>
#include <string.h>


static void PowerMeter_Callback(FDCANInstance *_ins){
    // 获取挂载在该CAN实例上的模块对象
    PowerMeter_t *meter  = (PowerMeter_t *)_ins->id;
    uint8_t *data = _ins->rx_buff;

    int16_t v_raw = (int16_t)((data[1] << 8) | data[0]);
    int16_t i_raw = (int16_t)((data[3] << 8) | data[2]);

    // 2. 物理转换（100倍缩放）
    meter->voltage = v_raw / 100.0f;
    meter->current = i_raw / 100.0f;
    meter->power = meter->voltage * meter->current;
}

PowerMeter_t* PowerMeter_Init(FDCAN_HandleTypeDef *hfdcan){
    PowerMeter_t *meter = (PowerMeter_t *)malloc(sizeof(PowerMeter_t));
    memset(meter,0,sizeof(PowerMeter_t));

    // 配置BSP 注册信息
    FDCAN_Init_Config_s config = {
        .fdcan_handle = hfdcan,
        .tx_id = 0, // 功率计通常被动发送
        .rx_id = 0x212, //匹配功率计反馈ID
        .data_len = 8,
        .fdcan_module_callback = PowerMeter_Callback, // 绑定解析逻辑
        .id = meter             // 将对象指针存入 id，方便回调函数找回

    };

    // 3. 注册并保存返回的实例句柄
    meter->can_ins = FDCANRegister(&config);

    return meter;
}