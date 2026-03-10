# RT-Thread GCC 移植问题手册

> 适用：STM32H723 + RT-Thread **v5.0.2** + arm-none-eabi-gcc 13.x + CLion/CMake

---

## 问题速查表

| # | 现象 | 根因 | 修复位置 |
|---|------|------|----------|
| 1 | 代码卡在 `rtthread_startup()`，线程不执行 | GCC 入口应为 `entry()` 非 `main()` | `startup.s` |
| 2 | `HAL_GetTick()` 永远返回 0，HAL 超时失效 | SysTick Handler 未调用 `HAL_IncTick()` | `board.c` |
| 3 | `HardFault` 或外设未初始化就运行 | HAL_Init/时钟配置未在 RT-Thread 接管前完成 | `board.c` |
| 4 | `multiple definition of 'PendSV_Handler'` | CubeMX 生成空桩与 `context_gcc.S` 冲突 | `stm32h7xx_it.c` |
| 5 | `multiple definition of 'HardFault_Handler'` | 同上 | `stm32h7xx_it.c` |
| 6 | `conflicting declaration 'typedef size_t rt_size_t'` | 项目自定义 typedef 与 `rtdef.h` 冲突 | `ringbuffer.h` |
| 7 | `fatal error: board.h: No such file or directory` | `cpu_cache.c` 需要 `board.h` 但 bsp 目录无此文件 | 新建 `board.h` |
| 8 | `fatal error: cpuport.h: No such file or directory` | v5.x `rthw.h` 需要 cpuport.h，路径未加入 include | `CMakeLists.txt` |
| 9 | `expected identifier before '{'`（cpu.c/cpu_cache.c） | 非 SMP 模式下函数名被宏替换/RT_USING_CACHE 未定义 | `CMakeLists.txt` + `rtconfig.h` |
| 10 | `expected ';' before 'void'`（board.c） | v5.x 弱符号宏改为小写 `rt_weak`（v3.x 是 `RT_WEAK`） | `board.c` |

> **注意**：问题 6（IT block）在 v5.0.2 中已由官方修复，无需手动处理。

---

## 详细修复

### 问题 1：启动流程错误

**根本原因**

RT-Thread 针对不同编译器有不同启动钩子，GCC 模式下入口是 `entry()`：

```c
// components.c（GCC 分支）
int entry(void) {
    rtthread_startup();  // 永不返回
    return 0;
}

// main_thread_entry 里才会调用 main()
void main_thread_entry(void *parameter) {
    rt_components_init();
    main();  // ← main() 在 RT-Thread 线程上下文中运行
}
```

如果 `startup.s` 调用 `main()`，`main()` 再调 `rtthread_startup()`，调度器启动后 main 线程又调 `main()`，形成无限递归。

**修复**

```asm
/* Core/Startup/startup_stm32h723vgtx.s */
- bl  main
+ bl  entry
```

**正确启动流程**

```
startup.s
  └─ bl entry
       └─ rtthread_startup()
            ├─ rt_hw_board_init()   ← HAL_Init + 时钟 + SysTick
            ├─ rt_application_init() ← 创建 main 线程
            └─ rt_system_scheduler_start() ← 永不返回
                   ↓ 调度器运行
            main_thread_entry()
              ├─ rt_components_init()  ← INIT_*_EXPORT 函数
              └─ main()               ← 外设初始化 + 创建业务线程
```

---

### 问题 2 + 3：SysTick 时基 & HAL 初始化位置

**修复 `board.c`**

```c
#include "stm32h7xx_hal.h"

extern void SystemClock_Config(void);
extern void PeriphCommonClock_Config(void);

void rt_hw_board_init(void)
{
    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();
    SystemCoreClockUpdate();
    _SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    HAL_IncTick();       /* ← 必须！保持 HAL 时基正常工作 */
    rt_interrupt_leave();
}
```

**`main.c` 对应调整**

```c
// main() 此时运行在 RT-Thread main 线程中
int main(void)
{
    MX_GPIO_Init();
    MX_DMA_Init();
    // ... 其他 MX_xxx_Init() ...

    app_main_init();
    rtt_app_threads_init();
    return 0;
}
```

---

### 问题 4 + 5：Handler 重复定义

| Handler | RT-Thread 实现位置 | 功能 |
|---------|-------------------|------|
| `PendSV_Handler` | `context_gcc.S` | 线程上下文切换 |
| `HardFault_Handler` | `context_gcc.S` | 栈溢出检测 |
| `SysTick_Handler` | `board.c` | tick + HAL_IncTick |

**修复：注释掉 `stm32h7xx_it.c` 中的三个函数**

```c
// void HardFault_Handler(void) { while(1){} }  /* 由 context_gcc.S 替代 */
// void PendSV_Handler(void) {}                  /* 由 context_gcc.S 替代 */
// void SysTick_Handler(void) { HAL_IncTick(); } /* 由 board.c 替代 */
```

---

### 问题 6：rt_size_t typedef 冲突

项目里 `ringbuffer.h` 为独立使用定义了 RT-Thread 兼容类型，集成后与 `rtdef.h` 冲突。

**修复 `ringbuffer.h`**

```c
#ifndef __RT_DEF_H__
typedef uint8_t     rt_uint8_t;
typedef uint16_t    rt_uint16_t;
typedef int16_t     rt_int16_t;
typedef size_t      rt_size_t;
#endif
```

---

### 问题 7：board.h 缺失

`cpu_cache.c` 通过 `board.h` 获取 `SCB_EnableICache()` 等 CMSIS 函数。

**新建 `bsp/board.h`**

```c
#ifndef __BOARD_H__
#define __BOARD_H__
#include "stm32h7xx_hal.h"
#endif
```

---

### 问题 8：cpuport.h 找不到（v5.x 新增）

v5.0.2 的 `rthw.h` 增加了 `#include <cpuport.h>`，文件在 `libcpu/arm/cortex-m7/` 目录。

**修复 `CMakeLists.txt`**（添加到 include_directories）：

```cmake
Middlewares/Third_Party/RealThread_RTOS_RT-Thread/libcpu/arm/cortex-m7
```

---

### 问题 9：cpu.c / cpu_cache.c 函数定义语法错误（v5.x 新增）

**原因 A（cpu_cache.c）**：`rthw.h` 在未定义 `RT_USING_CACHE` 时把
`rt_hw_cpu_icache_enable` 等替换为空宏，导致函数定义语法出错。

**修复**：在 `rtconfig.h` 加入：
```c
#define RT_USING_CACHE
```

**原因 B（cpu.c）**：`cpu.c` 包含 SMP 专属函数（`rt_cpu_self`、`rt_cpus_lock` 等），
在非 SMP 模式下引用未定义的 `struct rt_cpu / _cpus`；同时 `rtthread.h` 在非 SMP 下
把 `rt_spin_lock_init` 等替换为宏，与函数定义冲突。

**修复**：`CMakeLists.txt` 中排除此文件（STM32H7 单核，无需 SMP）：
```cmake
list(REMOVE_ITEM RT_THREAD_SOURCES
    "${CMAKE_SOURCE_DIR}/Middlewares/.../src/scheduler_mp.c"
    "${CMAKE_SOURCE_DIR}/Middlewares/.../src/cpu.c"
)
```

---

### 问题 10：`RT_WEAK` → `rt_weak`（v5.x API 变更）

v5.x 将弱符号宏统一为小写。`board.c` 中需将：

```c
RT_WEAK void *rt_heap_begin_get(void)  // ← v3.x 写法
```

改为：

```c
rt_weak void *rt_heap_begin_get(void)  // ← v5.x 写法
```

---

## v5.x CMakeLists.txt 关键配置

```cmake
# 头文件路径（比 v3.x 新增 libcpu/arm/cortex-m7）
include_directories(
    Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include
    Middlewares/Third_Party/RealThread_RTOS_RT-Thread/bsp
    Middlewares/Third_Party/RealThread_RTOS_RT-Thread/components/finsh
    Middlewares/Third_Party/RealThread_RTOS_RT-Thread/libcpu/arm/cortex-m7
)

# RT-Thread .c 源文件
file(GLOB_RECURSE RT_THREAD_SOURCES
    "Middlewares/Third_Party/RealThread_RTOS_RT-Thread/*.c")

# 排除 SMP 专属文件（单核 STM32H7）
list(REMOVE_ITEM RT_THREAD_SOURCES
    "${CMAKE_SOURCE_DIR}/Middlewares/.../src/scheduler_mp.c"
    "${CMAKE_SOURCE_DIR}/Middlewares/.../src/cpu.c"
)
list(APPEND SOURCES ${RT_THREAD_SOURCES})

# context_gcc.S 必须显式添加
list(APPEND SOURCES
    "Middlewares/Third_Party/RealThread_RTOS_RT-Thread/libcpu/arm/cortex-m7/context_gcc.S")
```

---

## CubeMX 重生成后检查清单

- [ ] `Core/Startup/startup_stm32h723vgtx.s`：确认是 `bl entry`
- [ ] `Core/Src/stm32h7xx_it.c`：确认 `PendSV_Handler` / `HardFault_Handler` / `SysTick_Handler` 仍被注释
- [ ] `Core/Src/main.c`：确认 USER CODE 块内容完整（app_main_init、rtt_app_threads_init）

> `Middlewares/` 目录 CubeMX **不会覆盖**，`board.c` / `context_gcc.S` 等修改是安全的。

---

## 堆大小说明

`board.c` 默认堆 **4KB**（`#define RT_HEAP_SIZE 1024`，单位 uint32_t）：

| 场景 | 建议堆大小 |
|------|-----------|
| 静态线程（`rt_thread_init`）| 4KB 够用 |
| 动态线程（`rt_thread_create`）| 16KB+ |
| FinSH + 多动态对象 | 32KB+ |

---

## 编译结果参考（v5.0.2，Debug 模式）

```
╔══════════════════════════════════════════════════════╗
║              Memory Usage  (STM32H723)              ║
╠══════════════════════════════════════════════════════╣
║  FLASH    [█░░░░░░░░░░░░░░░░░░░░░░░░░░░]   69.9 KB / 1.0 MB     6.8%  ║
║  DTCMRAM  [█████░░░░░░░░░░░░░░░░░░░░░░░]   27.1 KB / 128.0 KB  21.2%  ║
║  RAM_D1   [░░░░░░░░░░░░░░░░░░░░░░░░░░░░]    2.5 KB / 128.0 KB   2.0%  ║
╚══════════════════════════════════════════════════════╝
```
