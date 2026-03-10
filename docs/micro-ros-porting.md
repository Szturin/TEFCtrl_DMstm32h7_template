# micro-ROS 移植指南（STM32H7 + RT-Thread）

## 概述

本项目已集成 micro-ROS 适配层，支持 STM32H723 通过 UART 与 ROS 2 通信。

- **通信协议**：Micro XRCE-DDS over UART（DMA 循环缓冲 + RT-Thread 信号量）
- **编译方式**：条件编译，`libmicroros.a` 存在时自动启用
- **默认 UART**：USART3（可在 `micro_ros_transport.h` 修改）

---

## 目录结构

```
Middlewares/micro_ros/
├── micro_ros_transport.h      # UART 配置宏 + 4 个 transport 回调声明
├── micro_ros_transport.c      # transport 实现（DMA RX + 阻塞 TX）
├── micro_ros_allocators.c     # rcl_allocator_t 对接 rt_malloc
└── libmicroros/               # Docker 构建输出目录
    ├── libmicroros.a          # （运行 build_microros.sh 后生成）
    └── microros_include/      # micro-ROS 头文件

cmake/
└── build_microros.sh          # Docker 构建脚本

application/task/
├── micro_ros_task.h
└── micro_ros_task.c           # 示例：/stm32/motor_rpm publisher + /stm32/cmd_vel subscriber
```

---

## Step 1：构建 libmicroros.a（需要 Docker + WSL）

```bash
# 在 WSL 或 Linux 终端中运行（项目根目录）
chmod +x cmake/build_microros.sh
./cmake/build_microros.sh
```

构建完成后，以下文件自动生成：
- `Middlewares/micro_ros/libmicroros/libmicroros.a`
- `Middlewares/micro_ros/libmicroros/microros_include/`（所有 ROS 2 消息类型头文件）

> 如果没有 Docker，可以从 micro_ros_setup 的 Release 页面下载预编译版本（需匹配 MCU 架构 cortex-m7 + hard-fp）。

---

## Step 2：CubeMX 配置 USART3

1. **启用 USART3**
   - Mode: `Asynchronous`
   - Baud Rate: `460800`
   - Word Length: `8 Bits`
   - Stop Bits: `1`

2. **启用 DMA**（USART3_RX）
   - Mode: `Circular`
   - Direction: `Peripheral to Memory`
   - Data Width: `Byte`

3. **启用中断**
   - `USART3 global interrupt` ✔

4. **在 `stm32h7xx_it.c` 的 IRQHandler 中添加回调**：
   ```c
   #include "micro_ros_transport.h"

   void USART3_IRQHandler(void)
   {
       HAL_UART_IRQHandler(&huart3);
       microros_uart_rxcplt_callback();  /* 处理 IDLE 帧完成 */
   }
   ```

---

## Step 3：修改 UART 配置（可选）

编辑 `Middlewares/micro_ros/micro_ros_transport.h`：

```c
#define MICROROS_UART_HANDLE    huart3          /* 替换为实际 UART 句柄 */
#define MICROROS_UART_BAUDRATE  460800
#define MICROROS_RX_BUF_SIZE    512
#define MICROROS_TX_TIMEOUT_MS  100
```

---

## Step 4：编译

CMakeLists.txt 会自动检测 `libmicroros.a` 是否存在：

```
-- micro-ROS: libmicroros.a found — enabled
```

此时 `MICRO_ROS_ENABLED` 宏被定义，`app_main.cpp` 中的 microros 线程和 `micro_ros_task.c` 均参与编译。

**无 libmicroros.a 时**（默认状态）：

```
-- micro-ROS: libmicroros.a NOT found — disabled
```

`micro_ros_task.c` 中的 `#else` 分支提供空实现，工程正常编译。

---

## Step 5：PC 端启动 micro-ROS Agent

```bash
# ROS 2 环境（Humble / Iron / Jazzy 均可）
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 460800
```

Agent 启动后，STM32 会自动完成握手（`rmw_uros_ping_agent` 每 500ms 重试一次）。

---

## 话题说明

| 话题 | 方向 | 消息类型 | 说明 |
|------|------|----------|------|
| `/stm32/motor_rpm` | STM32 → PC | `std_msgs/Float32MultiArray` | 4 路电机转速，10ms 周期 |
| `/stm32/cmd_vel` | PC → STM32 | `geometry_msgs/Twist` | 速度指令（vx, vy, wz） |

---

## 线程配置

| 参数 | 值 | 说明 |
|------|----|------|
| 线程名 | `microros` | |
| 栈大小 | 8192 B | micro-ROS 内部 DDS 栈需求较大 |
| 优先级 | 15 | 低于控制线程（10-12），避免影响实时性 |

---

## rosserial vs micro-ROS

| 对比 | rosserial | micro-ROS（本项目） |
|------|-----------|---------------------|
| ROS 版本 | ROS 1 | ROS 2 |
| 协议 | 私有串口协议 | Micro XRCE-DDS（标准） |
| PC 工具 | `rosserial_python` | `micro_ros_agent` |
| 维护状态 | 已停止维护 | 官方支持 |
| 内存需求 | 更小 | 较大（~50KB+ heap） |

如果使用 ROS 2，选择 micro-ROS；如果是旧项目且只有 ROS 1，可参考 rosserial 方案。

---

## 常见问题

**Q: 编译报 `undefined reference to rcl_xxx`**
A: `libmicroros.a` 未找到，先运行 `./cmake/build_microros.sh`

**Q: Agent 连接后没有话题**
A: 检查 UART 波特率是否一致；检查 DMA IDLE 中断是否正确调用 `microros_uart_rxcplt_callback()`

**Q: 内存不足（rt_malloc 返回 NULL）**
A: 增大 `board.c` 中的 `RT_HEAP_SIZE`（当前 16KB，micro-ROS 初始化约需 40-60KB，建议 64KB 以上）

**Q: 如何修改发布的消息内容**
A: 编辑 `application/task/micro_ros_task.c` 中 `publish_timer_cb()` 函数，替换 `rpm_data` 数据来源
