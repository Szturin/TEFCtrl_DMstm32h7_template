# TEFCtrl STM32H7 模板工程

基于 STM32H723VGTx 的 RoboMaster 机器人控制板**通用模板**，支持 Keil MDK 和 CLion (CMake) 双构建系统。

## 硬件配置

- **MCU**: STM32H723VGTx
  - Cortex-M7 @ 480 MHz
  - 1MB FLASH / 320KB RAM（多 Bank：DTCMRAM / RAM_D1 等）
  - 双精度 FPU (fpv5-d16)
- **已配置外设**:
  - FDCAN1/2/3: CAN FD 通信（电机控制）
  - USART1/2/3/10, UART5/7: 串口通信
  - SPI1/2: 传感器通信（BMI088 IMU）
  - TIM1/2/3/12: 定时器（PWM/编码器）
  - USB Device CDC: USB 虚拟串口
  - ADC1/3, OCTOSPI2

## 软件架构

### 核心设计理念

- **CubeMX 隔离**：`Core/Src/main.c` 由 CubeMX 管理（纯 C），C++ 应用逻辑放在 `application/app_main.cpp`，CubeMX 重新生成不会破坏应用代码
- **C/C++ 混编**：BSP 层用 C，应用/驱动层用 C++，`extern "C"` 全面防护
- **双构建系统**：CMake（CLion）和 Keil MDK 均可编译，`sync_keil.py` 自动同步新增文件到 Keil 工程

### 目录结构

```
.
├── Core/                        # STM32CubeMX 生成（勿手动修改）
│   ├── Inc/
│   │   └── app_main.h           # C/C++ 接口桥接头文件
│   └── Src/
│       └── main.c               # 纯 C 入口（仅调用 app_main_init/run）
├── Drivers/                     # STM32 HAL 库 + CMSIS
├── Middlewares/
│   ├── ST/ARM/DSP/              # CMSIS-DSP（矩阵/滤波加速）
│   ├── ST/STM32_USB_Device_Library/  # USB CDC 中间件
│   └── Third_Party/RealThread_RTOS_RT-Thread/  # RT-Thread（预集成，可按需启用）
├── application/                 # 应用层（主要开发区域）
│   ├── app_main.cpp             # C++ 应用入口（全局对象 + 初始化）
│   ├── bsp/                     # 板级支持包
│   │   ├── can/                 # FDCAN 驱动（支持实例化注册）
│   │   ├── usart/               # UART 驱动
│   │   ├── delay/               # 延时（DWT）
│   │   └── BMI088/              # IMU 驱动
│   ├── app/                     # 应用模块
│   │   ├── general_def.h        # 全局常量（PI/RPM换算/电机参数）
│   │   ├── motor/
│   │   │   ├── dm_motor/        # 达妙电机驱动（MIT/位置/速度模式）
│   │   │   └── dji_motor/       # 大疆电机驱动（M2006/M3508/GM6020）
│   │   │       └── snail_c615   # Snail C615 ESC（TIM PWM）
│   │   ├── algorithm/           # 算法（PID / Mahony / IMU EKF）
│   │   ├── remote/              # 遥控器（WBUS/SBUS）
│   │   ├── ringbuffer/          # 环形缓冲区
│   │   └── vofa+/               # VOFA+ 调试协议
│   └── task/
│       ├── simple_os/           # 轻量级任务调度器（当前使用）
│       ├── motor_task.cpp       # 电机任务示例（模板请清空逻辑）
│       └── uart_task.c          # 串口通信任务
├── MDK-ARM/                     # Keil MDK 工程文件
├── sync_keil.py                 # 自动同步新增文件到 Keil 工程
└── CMakeLists.txt               # CMake 构建配置
```

### 应用入口机制

```
main.c (CubeMX 管理)
  ├─ app_main_init()  ──→  application/app_main.cpp
  └─ app_main_run()   ──→  application/app_main.cpp
                              ├─ 全局 C++ 对象初始化
                              ├─ BSP 初始化（CAN/UART/...）
                              ├─ simple_os 任务注册
                              └─ os.run() 进入调度循环
```

## 开发环境

| 工具 | 版本要求 | 说明 |
|------|---------|------|
| arm-none-eabi-gcc | 10.3+ | 交叉编译器 |
| CMake | 3.22+ | CLion 构建 |
| Keil MDK | 5.x | Keil 构建（`D:\keil5\UV4\UV4.exe`） |
| STM32CubeMX | 6.x | 外设配置 |
| Python 3 | 3.8+ | sync_keil.py 依赖 |

## 编译方法

### Keil MDK

打开 `MDK-ARM/TEFCtrl_DMstm32H7_template.uvprojx`，Rebuild。

命令行编译：
```bash
"D:\keil5\UV4\UV4.exe" -rebuild MDK-ARM/TEFCtrl_DMstm32H7_template.uvprojx
```

### CLion / CMake

```bash
mkdir cmake-build-debug && cd cmake-build-debug
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
make -j8
```

编译产物在 `cmake-build-debug/` 目录：
- `CtrBoard-H7_ALL.elf` / `.hex` / `.bin`

## 新增模块工作流

1. 在 `application/bsp/`、`application/app/`、`application/task/` 下添加文件
2. 运行 `python sync_keil.py` 自动更新 Keil 工程（无需手动在 Keil GUI 添加）
3. CMake 使用 `GLOB_RECURSE` 自动包含，无需修改 CMakeLists.txt

## CubeMX 重新生成注意事项

CubeMX 只修改 `Core/` 下的文件。每次重新生成后需检查：
1. `Core/Src/main.c` 中 USER CODE 区域是否保留（应包含 `app_main.h` 和 `app_main_init/run` 调用）
2. `Core/Src/main.c` 的文件类型在 Keil 中是否仍为 C（FileType=1）
3. 若 Keil 自动将 `main.c` 改为 C++ 类型，运行 `python sync_keil.py` 或手动修正

## 主要驱动模块

### 达妙（DM）电机
- 控制模式：MIT 力矩、位置、速度
- 通信：FDCAN @ 1Mbps
- 参考：`application/app/motor/dm_motor/`

### 大疆（DJI）电机
- 支持：M2006、M3508、GM6020
- 分组发送：0x200（M2006/M3508 1-4）、0x1FF（M2006/M3508 5-8）、0x2FF（GM6020 5-7）
- 参考：`application/app/motor/dji_motor/`

### Snail C615 ESC
- 控制：TIM2 CH1/CH3 PWM，1000~2000μs
- 参考：`application/app/motor/dji_motor/snail_c615.*`

### BMI088 IMU
- 接口：SPI
- 算法：Mahony 互补滤波 / 四元数 EKF

## RT-Thread 说明

RT-Thread 已集成在 `Middlewares/Third_Party/RealThread_RTOS_RT-Thread/` 中，**当前默认未启用**，使用轻量级 `simple_os` 调度器。

启用 RT-Thread 步骤（不需要修改 HAL 底层驱动，只需替换调度入口）：
1. 在 `app_main.cpp` 中将 `os.init()` / `os.run()` 替换为 RT-Thread 的 `rt_hw_board_init()` / `rt_system_scheduler_start()`
2. 用 `rt_thread_create()` 替换 `os.addTask()` 注册任务
3. 确保 `SysTick_Handler` 调用 `rt_tick_increase()`

## 常见问题

| 问题 | 解决方案 |
|------|---------|
| CMake: "created by incompatible generator" | 删除 cmake-build-debug 目录重新生成 |
| Keil: main.c 变成 C++ 类型 | sync_keil.py 中已处理，或手动将 FileType 改为 1 |
| DSP: `multiple definition of 'arm_mat_xxx'` | 只编译 `*Functions.c` 汇总文件，勿 GLOB 所有 .c |
| CAN 无响应 | 检查 `bsp_fdcan_set_baud` 和 `bsp_can_init` 调用顺序 |

## 技术规格

- C 标准：C11 / C++ 标准：C++17
- 编译器：arm-none-eabi-gcc（`-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard`）
- 调度器：simple_os（RT-Thread 预集成可切换）
- DSP：ARM CMSIS-DSP（MatrixFunctions / FastMath / BasicMath）
