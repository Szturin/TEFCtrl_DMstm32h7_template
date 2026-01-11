# STM32H723 RoboMaster 控制板项目

基于 STM32H723VGTx 的机器人控制系统，集成 RT-Thread RTOS、CMSIS-DSP 算法库和 DM 电机驱动。

## 硬件配置

- **MCU**: STM32H723VGTx
  - Cortex-M7 @ 480 MHz
  - 1MB FLASH / 320KB RAM
  - 双精度 FPU (fpv5-d16)
- **外设**:
  - FDCAN1/2: CAN FD 通信（电机控制）
  - UART7: 串口通信/日志输出
  - USB Device CDC: USB 虚拟串口
  - GPIO: 通用 IO

## 软件架构

### 核心组件

| 组件 | 版本 | 功能 |
|-----|------|------|
| RT-Thread | RTOS | 实时任务调度与资源管理 |
| CMSIS-DSP | ARM DSP Lib | 姿态解算、卡尔曼滤波 |
| STM32 HAL | STM32Cube | 硬件抽象层 |
| DM Motor Driver | C++ OOP | 达妙电机面向对象驱动 |

### 目录结构

```
.
├── Core/                   # STM32CubeMX 生成的核心代码
│   ├── Inc/               # 外设配置头文件
│   └── Src/               # 启动代码和初始化
├── Drivers/               # STM32 HAL 库和 CMSIS
├── Middlewares/
│   ├── ST/ARM/DSP/        # CMSIS-DSP 库（矩阵运算/滤波）
│   ├── ST/STM32_USB_Device_Library/  # USB Device 中间件
│   └── Third_Party/RealThread_RTOS_RT-Thread/  # RT-Thread RTOS
├── Application/           # 应用层代码
│   ├── bsp/              # 板级支持包（CAN/UART/USB）
│   ├── app/              # 应用模块
│   │   ├── motor/dm_motor/          # DM 电机驱动（C/C++ 混编）
│   │   ├── algorithm/_imu/          # IMU 姿态解算（EKF/Mahony）
│   │   └── algorithm/PID/           # PID 控制器
│   └── task/             # 任务层
│       ├── simple_os/    # 任务调度器
│       ├── motor_task.cpp # 电机控制任务
│       └── uart_task.c   # 串口通信任务
├── USB_DEVICE/            # USB 设备应用代码
└── CMakeLists.txt         # 构建配置
```

## 开发环境

### 必需工具

- **编译器**: arm-none-eabi-gcc 工具链 (推荐 10.3+)
- **构建工具**: CMake 3.31+
- **IDE**: CLion / VS Code / STM32CubeIDE（可选）

### 可选工具

- **STM32CubeMX**: 用于外设配置（修改 .ioc 文件）
- **烧录工具**: OpenOCD / ST-Link / J-Link

## 编译方法

### 使用 CMake（推荐）

```bash
mkdir build && cd build
cmake ..
make -j4
```

### 使用 CLion

直接打开项目目录，CLion 会自动识别 CMakeLists.txt。

### 编译产物

编译成功后在 `build` 或 `cmake-build-debug` 目录生成：
- `CtrBoard-H7_ALL.elf` - 可执行文件（包含调试符号）
- `CtrBoard-H7_ALL.hex` - 十六进制烧录文件
- `CtrBoard-H7_ALL.bin` - 二进制烧录文件
- `CtrBoard-H7_ALL.map` - 内存映射文件

## 烧录

使用 ST-Link / J-Link 等工具烧录 hex 或 bin 文件到目标板。

```bash
# OpenOCD 示例
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program build/CtrBoard-H7_ALL.elf verify reset exit"
```

## 主要功能

### 1. DM 电机驱动

- **实现方式**: C++ 面向对象封装（C 底层驱动 + C++ 接口层）
- **控制模式**: MIT、位置、速度、混合控制（PSI）
- **通信协议**: CAN FD @ 1Mbps
- **参考文档**: `Application/app/motor/dm_motor/DM电机CAN通信控制手册.md`

### 2. IMU 姿态解算

- **算法**:
  - Quaternion EKF（四元数扩展卡尔曼滤波）
  - Mahony AHRS（互补滤波）
- **依赖库**: CMSIS-DSP（矩阵运算加速）
- **输出**: Roll/Pitch/Yaw 欧拉角

### 3. 任务调度

- **方式**: RT-Thread 多任务调度 + 自定义 simple_os
- **任务**: 电机控制任务、串口通信任务、IMU 更新任务

## 配置说明

### 时钟配置

- HSI 64MHz → PLL 倍频 → 480MHz
- 配置函数: `SystemClock_Config()` (Core/Src/main.cpp)

### 电源模式

- 当前: LDO 供电模式 (`USE_PWR_LDO_SUPPLY`)
- 可选: SMPS 供电模式（修改宏定义）

### 内存保护

- MPU 已启用，修改内存映射需要调整 MPU 设置

### 编译优化

- Debug: `-Og` (调试优化)
- Release: `-Ofast` (最大速度优化)
- MinSizeRel: `-Os` (最小体积优化)

## C/C++ 混编规范

项目采用 C/C++ 混编架构：
- **底层驱动**: C 语言（HAL 库、BSP）
- **应用层**: C++ 语言（电机驱动封装、任务管理）

### 关键规范

1. **C 头文件必须使用 `extern "C"` 包裹**：
```c
#ifdef __cplusplus
extern "C" {
#endif

/* C 函数声明 */

#ifdef __cplusplus
}
#endif
```

2. **内存对齐**: CAN/DMA 缓冲区使用 `__attribute__((aligned(4)))`

3. **标准库使用**: 优先使用标准库函数（`sqrtf`, `atan2f`）而非编译器内建函数（`__sqrtf`）

## 常见问题

### 编译错误

| 错误 | 原因 | 解决方案 |
|-----|------|---------|
| `arm_math.h: No such file` | 未包含 DSP 头文件路径 | 检查 CMakeLists.txt 中 `Middlewares/ST/ARM/DSP/Include` |
| `multiple definition of 'arm_mat_xxx'` | DSP 源文件重复编译 | 只编译 `*Functions.c` 汇总文件，不要 GLOB 所有 .c |
| `undefined reference to '__sqrtf'` | 未链接数学库 | 添加 `target_link_libraries(...elf m)` |
| `cannot find entry symbol Reset_Handler` | 启动文件未编译 | 确保 GLOB 包含 `"Core/*.s"` |

### 运行异常

| 现象 | 可能原因 | 排查方法 |
|-----|---------|---------|
| FLASH 占用率异常低 (<10%) | 启动文件未链接 | 检查 `*.s` 文件是否编译 |
| CAN 通信无响应 | 波特率/过滤器配置错误 | 检查 `bsp_fdcan.c` 配置 |
| USB 虚拟串口无法识别 | USB 中间件未编译 | 检查 USB_MIDDLEWARE_SOURCES |

## 修改外设配置

1. 打开 `Test.ioc` 文件
2. 使用 STM32CubeMX 修改外设配置
3. 重新生成代码（保留 `USER CODE` 区域）
4. 如有新增外设，手动更新 CMakeLists.txt 中的 `include_directories`

## 技术规格

- **C 标准**: C11
- **C++ 标准**: C++17
- **编译器**: GCC 10.3+ (arm-none-eabi-gcc)
- **浮点模式**: 硬件浮点 (`-mfloat-abi=hard -mfpu=fpv5-d16`)
- **RTOS**: RT-Thread
- **DSP 库**: ARM CMSIS-DSP

## 许可证

本项目代码遵循各组件原有许可证：
- RT-Thread: Apache 2.0
- CMSIS-DSP: Apache 2.0
- STM32 HAL: BSD-3-Clause

## 参考资料

- [STM32H723 数据手册](https://www.st.com/en/microcontrollers-microprocessors/stm32h723vg.html)
- [RT-Thread 文档](https://www.rt-thread.org/document/site/)
- [CMSIS-DSP 库文档](https://arm-software.github.io/CMSIS_5/DSP/html/index.html)
- [DM 电机协议文档](Application/app/motor/dm_motor/DM电机CAN通信控制手册.md)
