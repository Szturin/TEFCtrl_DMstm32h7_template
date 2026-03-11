# TEFCtrl STM32H7 RT-Thread

基于 STM32H723VGTx 的 RoboMaster 机器人控制板，运行 **RT-Thread v5.0.2 RTOS**，支持 Keil MDK 和 CLion/VSCode (CMake) 构建系统。

## 硬件配置

- **MCU**: STM32H723VGTx
  - Cortex-M7 @ 480 MHz
  - 1MB FLASH / 320KB RAM（多 Bank：DTCMRAM / RAM_D1 等）
  - 双精度 FPU (fpv5-d16)
- **已配置外设**:
  - FDCAN1/2/3：CAN FD 通信（电机控制）
  - USART1/2/3/10, UART5/7：串口通信
  - SPI1/2：传感器通信（BMI088 IMU）
  - TIM1/2/3/12：定时器（PWM / 编码器）
  - USB Device CDC：USB 虚拟串口
  - ADC1/3, OCTOSPI2

## 软件架构

### 核心设计理念

- **RT-Thread RTOS**：v5.0.2 实时操作系统，抢占式多线程调度，FinSH 命令行调试
- **CubeMX 隔离**：`Core/Src/main.c` 由 CubeMX 管理（纯 C），C++ 应用逻辑在 `application/app_main.cpp`，重新生成不破坏应用代码
- **C/C++ 混编**：BSP 层用 C，应用/驱动层用 C++，`extern "C"` 隔离
- **双构建系统**：CMake（CLion/VSCode）和 Keil MDK 均可编译；`sync_keil.py` 自动同步新增文件到 Keil 工程

### 目录结构

```
.
├── Core/                          # STM32CubeMX 生成（勿手动修改）
│   ├── Inc/
│   │   └── app_main.h             # C/C++ 接口桥接头文件
│   └── Src/
│       └── main.c                 # 纯 C 入口（仅调 app_main_init/run）
├── Drivers/                       # STM32 HAL 库 + CMSIS
├── Middlewares/
│   ├── ST/ARM/DSP/                # CMSIS-DSP 源码（矩阵/三角函数加速）
│   ├── ST/STM32_USB_Device_Library/
│   └── Third_Party/RealThread_RTOS_RT-Thread/  # RT-Thread v5.0.2（已启用）
│       ├── bsp/                               #   board.c + rtconfig.h
│       ├── src/                               #   内核源码
│       ├── libcpu/arm/cortex-m7/              #   context_gcc.S 上下文切换
│       └── components/finsh/                  #   FinSH 命令行
├── application/                   # 应用层（主要开发区域）
│   ├── app_main.cpp               # C++ 应用入口
│   ├── app_link.h                 # 统一包含头（HAL + 常用 BSP）
│   ├── bsp/                       # 板级支持包
│   │   ├── can/                   # FDCAN 驱动（支持实例注册）
│   │   ├── usart/                 # UART DMA 驱动
│   │   ├── delay/                 # DWT 延时
│   │   └── BMI088/                # IMU SPI 驱动
│   ├── modules/                   # 功能模块（板级无关，可跨项目复用）
│   │   ├── general_def.h          # 全局常量（PI / RPM 换算 / 电机参数）
│   │   ├── motor/
│   │   │   ├── dm_motor/          # 达妙电机（MIT / 位置 / 速度模式）
│   │   │   ├── dji_motor/         # 大疆电机（M2006 / M3508 / GM6020）
│   │   │   ├── unitree_motor/     # 宇树电机（GO-M8010-6 / A1，RS485 4Mbps）
│   │   │   └── snail_motor/       # Snail 无刷电机（PWM）
│   │   ├── algorithm/
│   │   │   ├── controller/        # PID 控制器（位置式 + 增量式）
│   │   │   │   ├── pid.h/c        # 核心实现（微分先行/滤波/前馈/抗饱和）
│   │   │   │   └── pid_tune.h/c   # VOFA+ 串口滑块调参协议
│   │   │   ├── _imu/              # IMU 相关算法（EKF / Kalman）
│   │   │   ├── Mahony/            # Mahony 互补滤波
│   │   │   └── math/              # 数学工具
│   │   ├── daemon/                # 软件看门狗（掉线边沿触发回调）
│   │   ├── referee/               # 裁判系统通信
│   │   ├── remote/                # 遥控器（W-BUS / S-BUS）
│   │   ├── ringbuffer/            # 环形缓冲区
│   │   └── vofa+/                 # VOFA+ 调试上位机协议
│   └── task/                      # RT-Thread 线程
│       ├── motor_task.cpp         # 电机控制线程（DM + DJI）
│       ├── uart_task.cpp          # 串口 / 遥控器线程
│       ├── imu_temp_ctrl.c        # IMU 恒温控制
│       ├── micro_ros_task.c       # micro-ROS 通信（条件编译）
│       └── simple_os/             # 轻量级裸机调度器（备用）
├── MDK-ARM/                       # Keil MDK 工程文件
├── cmake/                         # CMake 工具链文件
├── sync_keil.py                   # 自动同步新增文件到 Keil 工程
└── CMakeLists.txt                 # CMake 构建配置
```

### 启动流程（RT-Thread GCC）

```
startup.s (bl entry)
  → entry()                         # components.c
    → rtthread_startup()
      → rt_hw_board_init()          # board.c: HAL_Init + 时钟 + SysTick 1kHz
      → rt_application_init()       # 创建 main 线程
      → rt_system_scheduler_start() # 永不返回
         ↓ 调度器
  → main_thread_entry()
    → rt_components_init()          # INIT_APP_EXPORT 注册函数
    → main()                        # Core/Src/main.c
      ├─ MX_xxx_Init()              # CubeMX 外设初始化
      ├─ app_main_init()            # BSP 初始化（CAN / UART / ...）
      └─ rtt_app_threads_init()     # 创建业务线程
```

### RT-Thread 线程配置

| 线程名 | 优先级 | 堆栈 | 周期 | 功能 |
|--------|--------|------|------|------|
| `motor` | 10 | 2KB | 1ms | DM 电机 + DJI M2006 速度闭环 |
| `unitree` | 11 | 2KB | 2ms | 宇树 GO-M8010-6 / A1 RS485 通信 |
| `uart` | 12 | 2KB | 5ms | 遥控器 UART 接收 |
| `microros` | 15 | 8KB | - | micro-ROS 通信（条件编译） |

## 开发环境

| 工具 | 版本要求 | 说明 |
|------|---------|------|
| arm-none-eabi-gcc | 10.3+ | 交叉编译器（CMake 构建） |
| CMake | 3.22+ | CLion / VSCode 构建 |
| Keil MDK | 5.x (AC6) | Keil 构建（`D:\keil5\UV4\UV4.exe`） |
| STM32CubeMX | 6.x | 外设配置 |
| Python 3 | 3.8+ | sync_keil.py 依赖 |

## 编译方法

### Keil MDK

打开 `MDK-ARM/TEFCtrl_DMstm32H7_template.uvprojx`，点击 Rebuild。

命令行：
```bash
"D:\keil5\UV4\UV4.exe" -rebuild MDK-ARM/TEFCtrl_DMstm32H7_template.uvprojx
```

### CLion / VSCode + CMake

```bash
mkdir cmake-build-debug && cd cmake-build-debug
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
ninja
```

VSCode 推荐插件：**CMake Tools** + **C/C++** + **Cortex-Debug**

### 新增文件后同步 Keil

```bash
python sync_keil.py
```

自动扫描 `application/bsp/`、`application/modules/`、`application/task/`，更新 Keil 工程的三个 Group，同时修复 DSP 库 Group。CMake 使用 `GLOB_RECURSE`，无需额外操作。

## CubeMX 重新生成注意事项

CubeMX 只修改 `Core/` 下的文件，应用代码安全。重新生成后**必须检查**：

1. **`startup.s`**：确认是 `bl entry`（不是 `bl main`），否则 RT-Thread 无法启动
2. **`stm32h7xx_it.c`**：确认 `PendSV_Handler` / `HardFault_Handler` / `SysTick_Handler` 仍被注释
3. **`main.c`**：确认 USER CODE 块内容完整（`app_main_init` + `rtt_app_threads_init` 调用）
4. Keil 中 `main.c` 的 FileType 是否仍为 1（C 文件）
5. 运行 `python sync_keil.py` 恢复 DSP 源码 Group（CubeMX 可能覆盖 `uvprojx`）

> 详见 `docs/cubemx-regen-fix.md`

## 主要驱动模块

### PID 控制器（`algorithm/controller/`）

位置式与增量式 PID，已集成：

| 优化项 | 说明 |
|--------|------|
| 微分先行 | D 项作用于反馈量，避免目标阶跃时微分跳变（位置式） |
| 微分低通滤波 | `D_alpha` 参数，抑制传感器高频噪声 |
| 积分分离 | `EIS_Max`：误差过大时停止积分 |
| 限幅抗积分饱和 | 输出饱和且同向时停止积分 |
| 前馈 | `FF_gain * target`，改进跟踪速度，减少输入输出相位差 |

所有限幅字段置 0 表示禁用。`pid_tune.c` 提供 VOFA+ 串口滑块调参协议（格式：`Kp:1.5\r\n`）。

### Topic 发布/订阅（`modules/topic/`）

轻量级线程间通信，基于 RT-Thread mutex + semaphore，支持多订阅者：

```c
// 定义 topic（.c 文件）
TOPIC_DEFINE(chassis_cmd, ChassisCmd_t);

// 声明 topic（.h 文件）
TOPIC_EXTERN(chassis_cmd);

// 发布（任意线程）
topic_publish(&chassis_cmd, &cmd);

// 订阅（阻塞等待新数据）
static topic_sub_t sub;
topic_sub_init(&sub, &chassis_cmd, "my_sub");
topic_sub_wait(&sub, &cmd, 10);   // 超时 10ms

// 轮询（读最新值，无需注册）
topic_peek(&chassis_cmd, &cmd);
```

### 软件看门狗（`modules/daemon/`）

- 静态内存池，无动态分配
- **边沿触发**：设备掉线仅触发一次回调，不重复调用（`is_online` 标志）
- 用法：`DaemonRegister()` 注册 → `DaemonReload()` 周期喂狗 → `DaemonTask()` 在调度器中调用

### 达妙（DM）电机（`motor/dm_motor/`）

- 控制模式：MIT 力矩、位置、速度、混合
- 通信：FDCAN @ 1Mbps

### 大疆（DJI）电机（`motor/dji_motor/`）

- 支持：M2006、M3508、GM6020、Snail C615（PWM）
- 分组：0x200（1-4）、0x1FF（5-8）、0x2FF（GM6020 5-7）

### 宇树（Unitree）电机（`motor/unitree_motor/`）

- 支持：GO-M8010-6（轮毂电机）、A1（关节电机）
- 通信：RS485 半双工 (USART3, 4Mbps, DE 硬件自动控制)
- 协议：GO 帧 17TX/16RX (CRC-CCITT-16)、A1 帧 34TX/78RX (CRC32)

### BMI088 IMU

- 接口：SPI；算法：Mahony 互补滤波 / 四元数 EKF

## CMSIS-DSP 说明

工程使用 **AC6（ARMCLANG）**，无法链接 AC5 格式预编译 `.lib`。已改为直接编译 DSP 源码，Keil `lib` Group 包含：

- `Source/MatrixFunctions/MatrixFunctions.c`
- `Source/FastMathFunctions/FastMathFunctions.c`
- `Source/CommonTables/CommonTables.c`

`sync_keil.py` 在 CubeMX 覆盖后自动恢复此配置。

## RT-Thread 配置

RT-Thread v5.0.2 已完整移植并启用，配置文件位于 `Middlewares/.../bsp/rtconfig.h`。

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `RT_TICK_PER_SECOND` | 1000 | 1ms tick |
| `RT_USING_HEAP` | 启用 | 动态内存分配 |
| `RT_USING_SMALL_MEM` | 启用 | 小内存管理算法 |
| `RT_HEAP_SIZE` | 16384 (uint32_t) | 64KB 堆（已从默认 4KB 增大） |
| `RT_USING_SMALL_MEM_AS_HEAP` | 启用 | RT-Thread 5.x 必需 |
| `RT_USING_FINSH` | 启用 | FinSH 命令行 |
| `RT_CONSOLE_DEVICE_NAME` | "uart1" | 日志输出串口 |
| `RT_USING_CACHE` | 启用 | ICache/DCache 管理 |

> 移植过程中解决的 10 个 GCC 兼容性问题详见 `docs/rt-thread-porting.md`

## 常见问题

| 问题 | 解决方案 |
|------|---------|
| 启动后不运行 / HardFault | 检查 `startup.s` 是否为 `bl entry`（CubeMX 会恢复为 `bl main`） |
| `multiple definition of PendSV_Handler` | 注释 `stm32h7xx_it.c` 中的 PendSV/HardFault/SysTick |
| `HAL_GetTick()` 返回 0 | 确认 `board.c` 的 `SysTick_Handler` 调用了 `HAL_IncTick()` |
| Keil: `L6007U` DSP lib 格式无法识别 | AC5 .lib 与 AC6 不兼容，运行 `sync_keil.py` 恢复源码编译 Group |
| Keil: `main.c` 变成 C++ 类型 | 手动将 FileType 改回 1，或运行 `sync_keil.py` |
| CMake: "incompatible generator" | 删除 `cmake-build-debug/` 重新 configure |
| CAN 无响应 | 检查 `bsp_fdcan_set_baud` 和 `bsp_can_init` 调用顺序（先 baud 再 init） |
| DSP: `multiple definition` | 只添加 `*Functions.c` 汇总文件，不要 GLOB 所有 `.c` |
| 堆不够用 | 增大 `board.c` 中 `RT_HEAP_SIZE`（默认 4KB，复杂项目需 16KB+） |

## 技术规格

- C 标准：C11 / C++ 标准：C++17
- 编译器（Keil）：ARMCLANG AC6，`-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard`
- 编译器（CMake）：arm-none-eabi-gcc，同等 FPU 配置
- 操作系统：**RT-Thread v5.0.2**（抢占式调度，FinSH 命令行）
- DSP：CMSIS-DSP 源码编译（Matrix / FastMath / CommonTables）

## 相关文档

| 文档 | 说明 |
|------|------|
| `docs/rt-thread-porting.md` | RT-Thread v5.0.2 GCC 移植问题手册（10 个问题 + 修复方案） |
| `docs/micro-ros-porting.md` | micro-ROS 集成指南（Docker 构建 libmicroros.a） |
| `docs/cubemx-regen-fix.md` | CubeMX 重生成后检查清单 |
| `application/modules/remote/Readme.md` | 遥控器 W-BUS 协议文档 |
| `application/task/simple_os/README.md` | 备用裸机调度器说明 |
