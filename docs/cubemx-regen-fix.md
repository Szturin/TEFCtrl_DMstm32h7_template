# CubeMX 重新生成后修复手册

CubeMX 重生成会覆盖三处 RT-Thread 关键改动，每次重生成后按此清单修复。

---

## 快速修复（3 步）

### Step 1：修复 startup.s 入口

文件：`Core/Startup/startup_stm32h723vgtx.s`，约第 101 行

```diff
-  bl  main
+  bl  entry
```

> 原因：GCC 模式下 RT-Thread 入口是 `entry()`（components.c），不是 `main()`。

---

### Step 2：注释 stm32h7xx_it.c 中的三个 Handler

文件：`Core/Src/stm32h7xx_it.c`

```c
// ① HardFault_Handler（约第 95 行）— 由 context_gcc.S 替代
// void HardFault_Handler(void) { while(1){} }

// ② PendSV_Handler（约第 181 行）— 由 context_gcc.S 替代
// void PendSV_Handler(void) {}

// ③ SysTick_Handler（约第 196 行）— 由 board.c 替代
// void SysTick_Handler(void) { HAL_IncTick(); }
```

> 原因：RT-Thread 的 `context_gcc.S` 和 `board.c` 提供了真实实现，CubeMX 空桩会导致链接错误。

---

### Step 3：确认 main.c USER CODE 内容

文件：`Core/Src/main.c`

CubeMX 会恢复 `main()` 中的 `HAL_Init()` / `SystemClock_Config()` 等调用，**但这是允许的**
（main() 运行在 RT-Thread 线程中，board.c 已完成初始化，HAL 重复初始化是幂等的）。

只需确认 USER CODE 块内容完整：

```c
/* USER CODE BEGIN Includes */
#include "app_main.h"           // ← 必须存在
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
app_main_init();                // ← 必须存在
rtt_app_threads_init();         // ← 必须存在
/* USER CODE END 2 */
```

---

## 不受影响的文件

以下文件在 `Middlewares/` 目录下，CubeMX **不会触碰**：

- `board.c` — HAL_Init + 时钟 + SysTick 双时基
- `rtconfig.h` — RT-Thread 配置（含 `RT_USING_CACHE`）
- `context_gcc.S` — v5.0.2 已内置 IT block，无需维护
- `board.h` — 自创建的 CMSIS 头桥接
- `ringbuffer.h` — `#ifndef __RT_DEF_H__` 保护

---

## CMakeLists.txt 是否受影响？

CubeMX 使用 `CMakeLists_template.txt` 重新生成 `CMakeLists.txt`。
该模板**已更新**，包含：
- RT-Thread 头文件路径（含 `libcpu/arm/cortex-m7` 提供 cpuport.h）
- `context_gcc.S` 显式添加
- 排除 SMP 专属文件（`scheduler_mp.c`, `cpu.c`）
- `memory_bar.py` post-build 调用

所以 CMakeLists.txt 重生成后**无需手动修改**。
