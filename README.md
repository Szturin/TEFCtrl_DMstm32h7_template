# TEFCtrl STM32H7 模板工程

基于 STM32H723VGTx 的 RoboMaster 机器人控制板**通用模板**，支持 Keil MDK 和 CLion/VSCode (CMake) 双构建系统。

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

- **CubeMX 隔离**：`Core/Src/main.c` 由 CubeMX 管理（纯 C），C++ 应用逻辑在 `application/app_main.cpp`，重新生成不破坏应用代码
- **C/C++ 混编**：BSP 层用 C，应用/驱动层用 C++，`extern "C"` 全面防护
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
│   └── Third_Party/RealThread_RTOS_RT-Thread/  # RT-Thread（预集成，默认未启用）
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
│   │   │   └── dji_motor/         # 大疆电机（M2006 / M3508 / GM6020）
│   │   │       └── snail_c615     # Snail C615 ESC（TIM PWM）
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
│   └── task/
│       ├── simple_os/             # 轻量级任务调度器（当前使用）
│       ├── motor_task.cpp         # 电机任务示例
│       ├── uart_task.cpp          # 串口 / 遥控器任务
│       └── imu_temp_ctrl.c        # IMU 恒温控制任务
├── MDK-ARM/                       # Keil MDK 工程文件
├── cmake/                         # CMake 工具链文件
├── sync_keil.py                   # 自动同步新增文件到 Keil 工程
└── CMakeLists.txt                 # CMake 构建配置
```

### 应用入口机制

```
main.c (CubeMX 管理)
  ├─ app_main_init()  ──→  application/app_main.cpp
  └─ app_main_run()   ──→  application/app_main.cpp
                              ├─ 全局 C++ 对象初始化
                              ├─ BSP 初始化（CAN / UART / ...）
                              ├─ simple_os 任务注册
                              └─ os.run() 进入调度循环
```

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

CubeMX 只修改 `Core/` 下的文件，应用代码安全。重新生成后：

1. 确认 `Core/Src/main.c` 中 USER CODE 保留（`app_main.h` 引用 + `app_main_init/run` 调用）
2. Keil 中 `main.c` 的 FileType 是否仍为 1（C 文件）
3. 运行 `python sync_keil.py` 恢复 DSP 源码 Group（CubeMX 可能覆盖 `uvprojx`）

## 主要驱动模块

### PID 控制器（`algorithm/controller/`）

位置式与增量式 PID，已集成：

| 优化项 | 说明 |
|--------|------|
| 微分先行 | D 项作用于反馈量，避免目标阶跃时微分跳变（位置式） |
| 微分低通滤波 | `D_alpha` 参数，抑制传感器高频噪声 |
| 积分分离 | `EIS_Max`：误差过大时停止积分 |
| 限幅抗积分饱和 | 输出饱和且同向时停止积分 |
| 前馈 | `FF_gain * target`，改善跟踪速度 |

所有限幅字段置 0 表示禁用。`pid_tune.c` 提供 VOFA+ 串口滑块调参协议（格式：`Kp:1.5\r\n`）。

### 软件看门狗（`modules/daemon/`）

- 静态内存池，无动态分配
- **边沿触发**：设备掉线仅触发一次回调，不重复调用（`is_online` 标志）
- 用法：`DaemonRegister()` 注册 → `DaemonReload()` 周期喂狗 → `DaemonTask()` 在调度器中调用

### 达妙（DM）电机（`motor/dm_motor/`）

- 控制模式：MIT 力矩、位置、速度
- 通信：FDCAN @ 1Mbps

### 大疆（DJI）电机（`motor/dji_motor/`）

- 支持：M2006、M3508、GM6020、Snail C615（PWM）
- 分组：0x200（1-4）、0x1FF（5-8）、0x2FF（GM6020 5-7）

### BMI088 IMU

- 接口：SPI；算法：Mahony 互补滤波 / 四元数 EKF

## CMSIS-DSP 说明

工程使用 **AC6（ARMCLANG）**，无法链接 AC5 格式预编译 `.lib`。已改为直接编译 DSP 源码，Keil `lib` Group 包含：

- `Source/MatrixFunctions/MatrixFunctions.c`
- `Source/FastMathFunctions/FastMathFunctions.c`
- `Source/CommonTables/CommonTables.c`

`sync_keil.py` 在 CubeMX 覆盖后自动恢复此配置。

## RT-Thread 说明

RT-Thread 已集成在 `Middlewares/Third_Party/RealThread_RTOS_RT-Thread/` 中，**默认未启用**，当前使用轻量级 `simple_os` 调度器。

切换 RT-Thread 只需替换调度入口（无需改 HAL 底层）：
1. `app_main.cpp` 中 `os.init/run()` 替换为 `rt_hw_board_init()` / `rt_system_scheduler_start()`
2. `os.addTask()` 替换为 `rt_thread_create()`
3. 确保 `SysTick_Handler` 调用 `rt_tick_increase()`

## 常见问题

| 问题 | 解决方案 |
|------|---------|
| Keil: `L6007U` DSP lib 格式无法识别 | AC5 .lib 与 AC6 不兼容，运行 `sync_keil.py` 恢复源码编译 Group |
| Keil: `main.c` 变成 C++ 类型 | 手动将 FileType 改回 1，或运行 `sync_keil.py` |
| CMake: "incompatible generator" | 删除 `cmake-build-debug/` 重新 configure |
| CAN 无响应 | 检查 `bsp_fdcan_set_baud` 和 `bsp_can_init` 调用顺序 |
| DSP: `multiple definition` | 只添加 `*Functions.c` 汇总文件，不要 GLOB 所有 `.c` |

## 技术规格

- C 标准：C11 / C++ 标准：C++17
- 编译器（Keil）：ARMCLANG AC6，`-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard`
- 编译器（CMake）：arm-none-eabi-gcc，同等 FPU 配置
- 调度器：**裸机协作式调度器 simple_os**（非抢占，基于 HAL_GetTick 轮询；RT-Thread 预集成可切换）
- DSP：CMSIS-DSP 源码编译（Matrix / FastMath / CommonTables）

## 裸机调度器说明（simple_os）

本工程为**裸机工程**，使用协作式（非抢占）调度器，无操作系统。

**运行机制**：主循环每毫秒扫描一次所有任务，满足 `经过时间 >= 周期` 则顺序执行。

**优先级**：数值越小越先执行，仅影响**同 tick 内的执行顺序**，不是抢占优先级。

**同周期同优先级**：多个任务按注册顺序依次执行，无冲突、无跳过。

**注意事项**：

| 规则 | 说明 |
|------|------|
| 任务函数不能阻塞 | 禁止在任务内使用 `HAL_Delay` / `while(1)` 等，否则卡死整个调度器 |
| 任务执行时间 < 周期 | 所有任务的执行时间之和不应超过最短任务周期，否则低优先级任务会漂移 |
| 时序精度 | 毫秒级，不保证微秒精度 |
