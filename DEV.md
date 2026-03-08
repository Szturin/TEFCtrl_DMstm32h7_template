# TEFCtrl STM32H7 — 开发上下文速查

## 项目定位

RoboMaster STM32H723VGTx 控制板**裸机模板**（无 RTOS，simple_os 轻量调度器）。
Keil MDK (AC6) + CLion/VSCode (CMake/GCC) 双构建系统。

## 关键路径

| 文件 | 说明 |
|------|------|
| `application/app_main.cpp` | C++ 应用入口，全局对象 + 任务注册 |
| `Core/Inc/app_main.h` | C/C++ 桥接头（extern "C" 保护） |
| `Core/Src/main.c` | CubeMX 管理，仅调用 app_main_init/run |
| `sync_keil.py` | 扫描 bsp/modules/task → 更新 uvprojx |
| `MDK-ARM/TEFCtrl_DMstm32H7_template.uvprojx` | Keil 工程文件 |

## 架构三层

```
bsp/          ← HAL 封装，纯 C，extern "C"
modules/      ← 功能模块，板级无关，C/C++ 混用
task/         ← 任务逻辑，C++，调用 modules
```

## 重要约定

- **所有 .h 文件必须有 `extern "C"` 保护**（会被 C 和 C++ 文件共同包含）
- **PID 结构体**：`PID_TypeDef`，位于 `modules/algorithm/controller/pid.h`，所有限幅字段 0=禁用
- **Daemon**：静态池，边沿触发（`is_online` 防重复），`DaemonTask()` 需在调度器中周期调用
- **DSP 库**：AC6 不能链接 AC5 .lib，已改为源码编译（MatrixFunctions.c + FastMathFunctions.c + CommonTables.c）

## sync_keil.py

- 扫描目录：`application/bsp/`, `application/modules/`, `application/task/`
- Keil Group：`user/bsp`, `user/modules`, `user/task`
- `fix_linker()` 恢复 DSP 源码 lib Group（防 CubeMX 覆盖）
- 新增文件后运行一次即可，无需手动在 Keil GUI 添加

## CubeMX 重生成后必做

1. 检查 `main.c` USER CODE 区域保留
2. 运行 `python sync_keil.py`（恢复 DSP Group）

## 常见坑

- `*.cmake` 不能用全局 glob ignore（会误删 cmake/ 工具链文件）
- FDCAN 需先 `bsp_fdcan_set_baud` 再 `bsp_can_init`
- `MDK-ARM/Test/` 是 Keil 编译输出目录，已加入 .gitignore
