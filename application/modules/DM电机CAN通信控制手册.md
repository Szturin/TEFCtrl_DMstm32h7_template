# DM电机CAN通信控制技术手册

## 目录
1. [系统概述](#系统概述)
2. [硬件配置](#硬件配置)
3. [软件架构](#软件架构)
4. [FDCAN通信层](#fdcan通信层)
5. [电机驱动层](#电机驱动层)
6. [电机控制层](#电机控制层)
7. [应用示例](#应用示例)
8. [关键API参考](#关键api参考)

---

## 系统概述

### 平台信息
- **MCU**: STM32H723
- **通信协议**: FDCAN (CAN FD with Bit Rate Switch)
- **电机型号**: DM系列电机
- **开发环境**: STM32 HAL库 裸机程序

### 功能特性
- 支持FDCAN通信,仲裁域1Mbps,数据域2Mbps
- 支持4种电机控制模式:MIT模式、位置模式、速度模式、混合模式
- 实时读取电机反馈数据:位置、速度、扭矩、温度
- 支持电机参数读写和配置保存
- 支持多电机控制(最多10个)

---

## 硬件配置

### FDCAN引脚配置
```
FDCAN1_RX: PD0
FDCAN1_TX: PD1
```

### 时钟配置
- **FDCAN时钟源**: PLL (80MHz)
- **系统时钟**: 400MHz

### 波特率配置
#### 仲裁段 (Nominal)
| 波特率 | Prescaler | Seg1 | Seg2 | SJW | 采样点 |
|--------|-----------|------|------|-----|--------|
| 125K   | 4         | 139  | 20   | 20  | 87.5%  |
| 250K   | 2         | 139  | 20   | 20  | 87.5%  |
| 500K   | 1         | 139  | 20   | 20  | 87.5%  |
| 1M     | 1         | 59   | 20   | 20  | 75%    |

#### 数据段 (Data - CAN FD BRS模式)
| 波特率 | Prescaler | Seg1 | Seg2 | SJW | 采样点 |
|--------|-----------|------|------|-----|--------|
| 2M     | 1         | 29   | 10   | 10  | 75%    |
| 2.5M   | 1         | 25   | 6    | 6   | 81.25% |
| 3.2M   | 1         | 19   | 5    | 5   | 80%    |
| 4M     | 1         | 14   | 5    | 5   | 75%    |
| 5M     | 1         | 13   | 2    | 2   | 87.5%  |

---

## 软件架构

### 分层结构
```
┌─────────────────────────────────┐
│      应用层 (main.c)              │  ← 定时器控制、业务逻辑
├─────────────────────────────────┤
│   电机控制层 (dm_motor_ctrl)      │  ← 初始化、数据处理、回调
├─────────────────────────────────┤
│   电机驱动层 (dm_motor_drv)       │  ← 控制模式、命令封装
├─────────────────────────────────┤
│   FDCAN BSP层 (bsp_fdcan)        │  ← 收发函数、滤波器、回调
├─────────────────────────────────┤
│   FDCAN HAL层 (fdcan.c)          │  ← STM32 HAL初始化
└─────────────────────────────────┘
```

### 文件结构
```
Core/
├── Src/
│   ├── main.c              # 主程序
│   ├── fdcan.c             # FDCAN硬件初始化
│   └── stm32h7xx_it.c      # 中断处理
└── Inc/
    ├── main.h
    ├── fdcan.h
    └── stm32h7xx_it.h

User/
├── bsp_fdcan.c             # FDCAN BSP层
├── bsp_fdcan.h
├── dm_motor_drv.c          # 电机驱动层
├── dm_motor_drv.h
├── dm_motor_ctrl.c         # 电机控制层
└── dm_motor_ctrl.h
```

---

## FDCAN通信层

### 初始化流程
文件位置: `bsp_fdcan.c`

```c
// 1. 设置波特率
bsp_fdcan_set_baud(&hfdcan1, CAN_CLASS, CAN_BR_1M);

// 2. 初始化CAN(滤波器+启动+中断)
bsp_can_init();
```

### 滤波器配置
```c
void can_filter_init(void)
{
    FDCAN_FilterTypeDef fdcan_filter;

    // 标准ID滤波器,接收所有消息
    fdcan_filter.IdType = FDCAN_STANDARD_ID;
    fdcan_filter.FilterIndex = 0;
    fdcan_filter.FilterType = FDCAN_FILTER_MASK;
    fdcan_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    fdcan_filter.FilterID1 = 0x00;  // 接收所有ID
    fdcan_filter.FilterID2 = 0x00;

    HAL_FDCAN_ConfigFilter(&hfdcan1, &fdcan_filter);
}
```

### 发送函数
文件位置: `bsp_fdcan.c:117`

```c
uint8_t fdcanx_send_data(hcan_t *hfdcan, uint16_t id, uint8_t *data, uint32_t len)
{
    FDCAN_TxHeaderTypeDef pTxHeader;

    pTxHeader.Identifier = id;
    pTxHeader.IdType = FDCAN_STANDARD_ID;
    pTxHeader.TxFrameType = FDCAN_DATA_FRAME;
    pTxHeader.DataLength = len;  // 支持8/12/16/20/24/32/48/64字节
    pTxHeader.BitRateSwitch = FDCAN_BRS_ON;  // 开启BRS
    pTxHeader.FDFormat = FDCAN_FD_CAN;

    if(HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &pTxHeader, data) != HAL_OK)
        return 1;
    return 0;
}
```

### 接收函数
文件位置: `bsp_fdcan.c:160`

```c
uint8_t fdcanx_receive(hcan_t *hfdcan, uint16_t *rec_id, uint8_t *buf)
{
    FDCAN_RxHeaderTypeDef pRxHeader;
    uint8_t len;

    if(HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &pRxHeader, buf) == HAL_OK)
    {
        *rec_id = pRxHeader.Identifier;
        len = pRxHeader.DataLength;  // 解析数据长度
        return len;
    }
    return 0;
}
```

### 接收中断回调
文件位置: `bsp_fdcan.c:197`

```c
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan == &hfdcan1)
    {
        fdcan1_rx_callback();  // 用户自定义回调
    }
}
```

---

## 电机驱动层

### 数据结构定义
文件位置: `dm_motor_drv.h`

#### 控制模式枚举
```c
typedef enum
{
    mit_mode = 1,  // MIT模式 (位置+速度+扭矩混合控制)
    pos_mode = 2,  // 位置模式
    spd_mode = 3,  // 速度模式
    psi_mode = 4   // 混合模式 (位置+速度+电流)
} mode_e;
```

#### 电机结构体
```c
typedef struct
{
    uint16_t id;              // 电机ID
    uint16_t mst_id;          // 主机ID
    motor_fbpara_t para;      // 反馈参数
    motor_ctrl_t ctrl;        // 控制参数
    esc_inf_t tmp;            // 电机配置参数
} motor_t;
```

#### 反馈参数结构
```c
typedef struct
{
    int id;          // 电机ID
    int state;       // 电机状态
    int p_int;       // 位置原始值
    int v_int;       // 速度原始值
    int t_int;       // 扭矩原始值
    float pos;       // 位置(rad)
    float vel;       // 速度(rad/s)
    float tor;       // 扭矩(N·m)
    float Kp;        // 当前Kp
    float Kd;        // 当前Kd
    float Tmos;      // MOS管温度(°C)
    float Tcoil;     // 线圈温度(°C)
} motor_fbpara_t;
```

#### 控制参数结构
```c
typedef struct
{
    uint8_t mode;     // 控制模式
    float pos_set;    // 位置设定值(rad)
    float vel_set;    // 速度设定值(rad/s)
    float tor_set;    // 扭矩设定值(N·m)
    float cur_set;    // 电流设定值(A)
    float kp_set;     // Kp设定值
    float kd_set;     // Kd设定值
} motor_ctrl_t;
```

### 核心控制函数

#### 使能电机
文件位置: `dm_motor_drv.c:15`

```c
void dm_motor_enable(hcan_t* hcan, motor_t *motor)
{
    // 根据控制模式发送使能命令
    // 发送: 0xFFFFFFFFFFFFFFFC
    enable_motor_mode(hcan, motor->id, mode_id);
}
```

#### 失能电机
文件位置: `dm_motor_drv.c:43`

```c
void dm_motor_disable(hcan_t* hcan, motor_t *motor)
{
    // 根据控制模式发送失能命令
    // 发送: 0xFFFFFFFFFFFFFFFD
    disable_motor_mode(hcan, motor->id, mode_id);
    dm_motor_clear_para(motor);  // 清除控制参数
}
```

#### MIT模式控制
文件位置: `dm_motor_drv.c:315`

```c
void mit_ctrl(hcan_t* hcan, motor_t *motor, uint16_t motor_id,
              float pos, float vel, float kp, float kd, float tor)
{
    // CAN ID = motor_id + 0x000
    // 数据帧格式(8字节):
    // [0-1]: 位置 (16bit, -PMAX~PMAX)
    // [2-3]: 速度高位 (12bit, -VMAX~VMAX)
    // [3-4]: Kp (12bit, 0~500)
    // [5-6]: Kd高位 (12bit, 0~5)
    // [6-7]: 扭矩 (12bit, -TMAX~TMAX)
}
```

#### 位置模式控制
文件位置: `dm_motor_drv.c:348`

```c
void pos_ctrl(hcan_t* hcan, uint16_t motor_id, float pos, float vel)
{
    // CAN ID = motor_id + 0x100
    // 数据帧格式(8字节):
    // [0-3]: 位置 (float, rad)
    // [4-7]: 速度 (float, rad/s)
}
```

#### 速度模式控制
文件位置: `dm_motor_drv.c:380`

```c
void spd_ctrl(hcan_t* hcan, uint16_t motor_id, float vel)
{
    // CAN ID = motor_id + 0x200
    // 数据帧格式(4字节):
    // [0-3]: 速度 (float, rad/s)
}
```

#### 混合模式控制
文件位置: `dm_motor_drv.c:409`

```c
void psi_ctrl(hcan_t* hcan, uint16_t motor_id, float pos, float vel, float cur)
{
    // CAN ID = motor_id + 0x300
    // 数据帧格式(8字节):
    // [0-3]: 位置 (float, rad)
    // [4-5]: 速度×100 (uint16_t)
    // [6-7]: 电流×10000 (uint16_t)
}
```

### 参数读写函数

#### 读取寄存器
文件位置: `dm_motor_drv.c:445`

```c
void read_motor_data(uint16_t id, uint8_t rid)
{
    // CAN ID: 0x7FF (广播地址)
    // 数据格式: [id_low, id_high, 0x33, rid]
    uint8_t data[4] = {id&0x0F, (id>>4)&0x0F, 0x33, rid};
    fdcanx_send_data(&hfdcan1, 0x7FF, data, 4);
}
```

#### 写入寄存器
文件位置: `dm_motor_drv.c:479`

```c
void write_motor_data(uint16_t id, uint8_t rid, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    // CAN ID: 0x7FF
    // 数据格式: [id_low, id_high, 0x55, rid, d0, d1, d2, d3]
    uint8_t data[8] = {id&0xFF, (id>>8)&0x07, 0x55, rid, d0, d1, d2, d3};
    fdcanx_send_data(&hfdcan1, 0x7FF, data, 8);
}
```

#### 保存参数到Flash
文件位置: `dm_motor_drv.c:496`

```c
void save_motor_data(uint16_t id, uint8_t rid)
{
    // CAN ID: 0x7FF
    // 数据格式: [id_low, id_high, 0xAA, 0x01]
    uint8_t data[4] = {id&0xFF, (id>>8)&0x07, 0xAA, 0x01};
    fdcanx_send_data(&hfdcan1, 0x7FF, data, 4);
}
```

### 反馈数据解析
文件位置: `dm_motor_drv.c:146`

```c
void dm_motor_fbdata(motor_t *motor, uint8_t *rx_data)
{
    // 从CAN反馈帧中提取数据
    motor->para.id = (rx_data[0]) & 0x0F;
    motor->para.state = (rx_data[0]) >> 4;
    motor->para.p_int = (rx_data[1]<<8) | rx_data[2];
    motor->para.v_int = (rx_data[3]<<4) | (rx_data[4]>>4);
    motor->para.t_int = ((rx_data[4]&0xF)<<8) | rx_data[5];

    // 转换为物理量
    motor->para.pos = uint_to_float(motor->para.p_int, -PMAX, PMAX, 16);
    motor->para.vel = uint_to_float(motor->para.v_int, -VMAX, VMAX, 12);
    motor->para.tor = uint_to_float(motor->para.t_int, -TMAX, TMAX, 12);
    motor->para.Tmos = (float)(rx_data[6]);
    motor->para.Tcoil = (float)(rx_data[7]);
}
```

---

## 电机控制层

### 电机初始化
文件位置: `dm_motor_ctrl.c:18`

```c
void dm_motor_init(void)
{
    // 清零所有电机结构体
    memset(&motor[Motor1], 0, sizeof(motor[Motor1]));

    // 配置电机1参数
    motor[Motor1].id = 0x01;
    motor[Motor1].mst_id = 0x00;
    motor[Motor1].tmp.read_flag = 1;
    motor[Motor1].ctrl.mode = psi_mode;
    motor[Motor1].ctrl.vel_set = 1.0f;
    motor[Motor1].ctrl.pos_set = 6.28f;
    motor[Motor1].ctrl.tor_set = 0.0f;
    motor[Motor1].ctrl.cur_set = 0.02f;
    motor[Motor1].ctrl.kp_set = 0.0f;
    motor[Motor1].ctrl.kd_set = 0.0f;

    // 设置电机物理参数
    motor[Motor1].tmp.PMAX = 12.5f;  // 位置范围(rad)
    motor[Motor1].tmp.VMAX = 30.0f;  // 速度范围(rad/s)
    motor[Motor1].tmp.TMAX = 10.0f;  // 扭矩范围(N·m)
}
```

### 读取所有参数
文件位置: `dm_motor_ctrl.c:51`

```c
void read_all_motor_data(motor_t *motor)
{
    // 状态机顺序读取45个寄存器
    switch (motor->tmp.read_flag)
    {
        case 1:  read_motor_data(motor->id, RID_UV_VALUE);  break; // 欠压保护值
        case 2:  read_motor_data(motor->id, RID_KT_VALUE);  break; // 扭矩系数
        case 3:  read_motor_data(motor->id, RID_OT_VALUE);  break; // 过温保护值
        // ... 共45个参数
        case 45: read_motor_data(motor->id, RID_X_OUT);     break; // 输出位置
    }
}
```

### 接收参数反馈
文件位置: `dm_motor_ctrl.c:111`

```c
void receive_motor_data(motor_t *motor, uint8_t *data)
{
    if(motor->tmp.read_flag == 0)
        return;

    float_type_u y;

    if(data[2] == 0x33)  // 读寄存器响应
    {
        uint16_t rid_value = data[3];
        y.b_val[0] = data[4];
        y.b_val[1] = data[5];
        y.b_val[2] = data[6];
        y.b_val[3] = data[7];

        // 根据寄存器ID保存数据并更新状态
        switch (rid_value)
        {
            case RID_UV_VALUE:
                motor->tmp.UV_Value = y.f_val;
                motor->tmp.read_flag = 2;
                break;
            // ... 其他寄存器
        }
    }
}
```

### CAN接收回调
文件位置: `dm_motor_ctrl.c:186`

```c
void fdcan1_rx_callback(void)
{
    uint16_t rec_id;
    uint8_t rx_data[8] = {0};

    fdcanx_receive(&hfdcan1, &rec_id, rx_data);

    switch (rec_id)
    {
        case 0x00:
            dm_motor_fbdata(&motor[Motor1], rx_data);      // 解析反馈
            receive_motor_data(&motor[Motor1], rx_data);   // 处理参数
            break;
        // 可添加更多电机
    }
}
```

---

## 应用示例

### 主程序初始化
文件位置: `main.c:78`

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    // 外设初始化
    MX_GPIO_Init();
    MX_FDCAN1_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();
    MX_TIM4_Init();

    // 上电并延时
    power(1);
    HAL_Delay(1000);

    // 配置FDCAN波特率: 仲裁域1M, 数据域2M
    bsp_fdcan_set_baud(&hfdcan1, CAN_CLASS, CAN_BR_1M);

    // 初始化CAN和电机
    bsp_can_init();
    dm_motor_init();
    motor[Motor1].ctrl.mode = mit_mode;
    HAL_Delay(100);

    // 配置电机参数
    write_motor_data(motor[Motor1].id, 10, mit_mode, 0, 0, 0);
    HAL_Delay(100);

    // 读取CAN波特率
    read_motor_data(motor[Motor1].id, RID_CAN_BR);

    // 失能电机
    dm_motor_disable(&hfdcan1, &motor[Motor1]);
    HAL_Delay(100);

    // 保存参数
    save_motor_data(motor[Motor1].id, 10);
    HAL_Delay(100);

    // 使能电机
    dm_motor_enable(&hfdcan1, &motor[Motor1]);
    HAL_Delay(1000);

    // 启动定时器中断(1ms周期)
    HAL_TIM_Base_Start_IT(&htim3);

    while (1)
    {
        // 主循环
    }
}
```

### 定时器控制
文件位置: `main.c:61`

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        // 读取电机所有参数
        read_all_motor_data(&motor[Motor1]);

        // 参数读取完成后发送控制命令
        if(motor[Motor1].tmp.read_flag == 0)
            dm_motor_ctrl_send(&hfdcan1, &motor[Motor1]);
    }
}
```

---

## 关键API参考

### FDCAN BSP层API

| 函数名 | 功能 | 文件位置 |
|--------|------|----------|
| `bsp_can_init()` | 初始化CAN(滤波器+中断) | bsp_fdcan.c:10 |
| `can_filter_init()` | 配置CAN滤波器 | bsp_fdcan.c:35 |
| `bsp_fdcan_set_baud()` | 动态设置波特率 | bsp_fdcan.c:54 |
| `fdcanx_send_data()` | 发送CAN数据 | bsp_fdcan.c:117 |
| `fdcanx_receive()` | 接收CAN数据 | bsp_fdcan.c:160 |
| `fdcan1_rx_callback()` | CAN接收回调(弱定义) | bsp_fdcan.c:192 |

### 电机驱动层API

| 函数名 | 功能 | 文件位置 |
|--------|------|----------|
| `dm_motor_enable()` | 使能电机 | dm_motor_drv.c:15 |
| `dm_motor_disable()` | 失能电机 | dm_motor_drv.c:43 |
| `dm_motor_ctrl_send()` | 发送控制命令 | dm_motor_drv.c:72 |
| `dm_motor_clear_para()` | 清除控制参数 | dm_motor_drv.c:100 |
| `dm_motor_clear_err()` | 清除电机错误 | dm_motor_drv.c:118 |
| `dm_motor_fbdata()` | 解析反馈数据 | dm_motor_drv.c:146 |
| `mit_ctrl()` | MIT模式控制 | dm_motor_drv.c:315 |
| `pos_ctrl()` | 位置模式控制 | dm_motor_drv.c:348 |
| `spd_ctrl()` | 速度模式控制 | dm_motor_drv.c:380 |
| `psi_ctrl()` | 混合模式控制 | dm_motor_drv.c:409 |
| `read_motor_data()` | 读取寄存器 | dm_motor_drv.c:445 |
| `write_motor_data()` | 写入寄存器 | dm_motor_drv.c:479 |
| `save_motor_data()` | 保存参数 | dm_motor_drv.c:496 |

### 电机控制层API

| 函数名 | 功能 | 文件位置 |
|--------|------|----------|
| `dm_motor_init()` | 初始化电机结构 | dm_motor_ctrl.c:18 |
| `read_all_motor_data()` | 读取所有寄存器 | dm_motor_ctrl.c:51 |
| `receive_motor_data()` | 接收参数反馈 | dm_motor_ctrl.c:111 |
| `fdcan1_rx_callback()` | CAN接收回调实现 | dm_motor_ctrl.c:186 |

---

## 寄存器地址定义

### 常用寄存器(RID)
文件位置: `dm_motor_drv.h:40`

| 寄存器ID | 名称 | 说明 | 类型 |
|----------|------|------|------|
| 0 | RID_UV_VALUE | 欠压保护值 | float |
| 1 | RID_KT_VALUE | 扭矩系数 | float |
| 2 | RID_OT_VALUE | 过温保护值 | float |
| 3 | RID_OC_VALUE | 过流保护值 | float |
| 4 | RID_ACC | 加速度 | float |
| 5 | RID_DEC | 减速度 | float |
| 6 | RID_MAX_SPD | 最大速度 | float |
| 7 | RID_MST_ID | 主机ID | uint32 |
| 8 | RID_ESC_ID | 电机ID | uint32 |
| 10 | RID_CMODE | 控制模式 | uint32 |
| 21 | RID_PMAX | 位置映射范围 | float |
| 22 | RID_VMAX | 速度映射范围 | float |
| 23 | RID_TMAX | 扭矩映射范围 | float |
| 25 | RID_KP_ASR | 速度环Kp | float |
| 26 | RID_KI_ASR | 速度环Ki | float |
| 27 | RID_KP_APR | 位置环Kp | float |
| 28 | RID_KI_APR | 位置环Ki | float |
| 35 | RID_CAN_BR | CAN波特率配置 | uint32 |

---

## CAN通信协议

### 控制帧ID定义
```
MIT模式:  motor_id + 0x000
位置模式: motor_id + 0x100
速度模式: motor_id + 0x200
混合模式: motor_id + 0x300
```

### 特殊命令帧
| CAN ID | 命令字(Byte7) | 功能 |
|--------|---------------|------|
| motor_id+mode | 0xFC | 使能电机 |
| motor_id+mode | 0xFD | 失能电机 |
| motor_id+mode | 0xFE | 保存零点 |
| motor_id+mode | 0xFB | 清除错误 |

### 参数读写帧
| CAN ID | Byte2 | 功能 |
|--------|-------|------|
| 0x7FF | 0x33 | 读寄存器 |
| 0x7FF | 0x55 | 写寄存器 |
| 0x7FF | 0xAA | 保存参数 |

### 反馈帧格式(接收ID: 0x00)
```
Byte0[7:4]: 电机状态
Byte0[3:0]: 电机ID
Byte1-2:    位置(16bit)
Byte3-4:    速度(12bit)
Byte4-5:    扭矩(12bit)
Byte6:      MOS温度
Byte7:      线圈温度
```

---

## 使用注意事项

### 1. 初始化顺序
```c
// 正确顺序
power(1);                    // 上电
HAL_Delay(1000);             // 等待稳定
bsp_fdcan_set_baud();        // 设置波特率
bsp_can_init();              // 初始化CAN
dm_motor_init();             // 初始化电机
dm_motor_enable();           // 使能电机
HAL_TIM_Base_Start_IT();     // 启动定时控制
```

### 2. 参数配置
- **PMAX/VMAX/TMAX**: 必须在使用前设置,用于数据映射
- **控制模式**: 使能电机前必须设置`motor->ctrl.mode`
- **控制周期**: 建议1ms定时器中断

### 3. 通信注意
- 参数读取是状态机循环,每次中断读一个寄存器
- `read_flag=0`时才发送控制命令,避免冲突
- 写入参数后必须调用`save_motor_data()`保存到Flash

### 4. 错误处理
```c
// 总线错误恢复
HAL_FDCAN_ErrorStatusCallback()  // 自动处理BusOff和ErrorPassive

// 电机错误清除
dm_motor_clear_err(&hfdcan1, &motor[Motor1]);
```

### 5. 多电机控制
```c
// 在fdcan1_rx_callback()中添加更多case
switch (rec_id)
{
    case 0x00: /* 电机1 */ break;
    case 0x01: /* 电机2 */ break;
    case 0x02: /* 电机3 */ break;
}
```

---

## 版本信息

- **工程版本**: v1.1
- **MCU**: STM32H723
- **HAL库版本**: STM32H7xx HAL
- **通信协议**: FDCAN with BRS
- **控制模式**: MIT/POS/SPD/PSI

---

**文档生成时间**: 2026-01-04
**作者**: 根据源码分析生成
