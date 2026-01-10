# STM32H723 测试项目

基于 STM32H723VGTx 的嵌入式开发工程。

## 硬件配置

- MCU: STM32H723VGTx
- 主频: 480 MHz (HSI 64MHz PLL 倍频)
- 外设:
  - UART7: 串口通信/日志输出
  - FDCAN1: CAN FD 通信
  - GPIO: 通用 IO

## 目录结构

```
.
├── Core/              # STM32CubeMX 生成的核心代码
│   ├── Inc/          # 外设配置头文件
│   └── Src/          # 启动和初始化代码
├── Drivers/          # STM32 HAL 库和 CMSIS
├── Application/      # 应用层代码
│   ├── bsp/         # 板级支持包
│   └── main.cpp     # 主程序
├── cmake/           # CMake 配置文件
└── *.ld             # 链接脚本
```

## 编译环境

需要安装:
- arm-none-eabi-gcc 工具链
- CMake 3.31+
- STM32CubeMX (可选,用于修改外设配置)

## 编译方法

使用 CMake 命令行:
```bash
mkdir build && cd build
cmake ..
make
```

或者直接用 CLion/STM32CubeIDE 打开项目。

编译产物在 `build` 目录:
- `Test.elf` - 可执行文件
- `Test.hex` - 十六进制烧录文件
- `Test.bin` - 二进制烧录文件

## 烧录

使用 OpenOCD/ST-Link/J-Link 等工具烧录 hex 或 bin 文件。

## 当前功能

演示程序通过 UART7 输出正弦波数值,用于验证浮点运算和串口通信。

## 修改外设配置

1. 打开 `Test.ioc` 文件
2. 用 STM32CubeMX 修改配置
3. 重新生成代码 (保留 USER CODE 区域内容)

## 注意事项

- 时钟配置在 `SystemClock_Config()` 里
- MPU 已启用,修改内存映射需要调整 MPU 设置
- 默认使用 LDO 供电模式
- C++ 标准: C++17, C 标准: C11
