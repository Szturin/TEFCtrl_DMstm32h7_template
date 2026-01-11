# ==============================================================================
# 此文件针对 STM32H723 + RT-Thread + CMSIS-DSP 进行了优化
# ==============================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.31)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(CtrBoard-H7_ALL C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# --- [关键修复：开启 H7 硬件浮点支持] ---
# STM32H723 使用的是双精度 FPU (fpv5-d16) 或单精度 (fpv5-sp-d16)
# 这里使用通用的 cortex-m7 配置
add_compile_options(-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
add_link_options(-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)

add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

# --- [关键修复：宏定义] ---
# 1. 增加 ARM_MATH_CM7 用于支持 DSP 库
# 2. 增加 __FPU_PRESENT=1 确保开启浮点加速
add_definitions(
        -DDEBUG
        -DUSE_PWR_LDO_SUPPLY
        -DUSE_HAL_DRIVER
        -DSTM32H723xx
        -DARM_MATH_CM7
        -DARM_MATH_MATRIX_CHECK
        -DARM_MATH_ROUNDING
)

# --- [关键修复：头文件路径搜索] ---
# 注意：如果 bsp_dwt.h 在 Application/bsp/dwt 这样的子目录下，你需要继续追加
include_directories(
        Core/Inc
        USB_DEVICE/App
        USB_DEVICE/Target
        Drivers/STM32H7xx_HAL_Driver/Inc
        Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
        Middlewares/ST/STM32_USB_Device_Library/Core/Inc
        Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
        Drivers/CMSIS/Device/ST/STM32H7xx/Include
        Drivers/CMSIS/Include
        Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/
        Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/include/
        Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/
        Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/fcntl/msvc/
        Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/fcntl/octal/
        Middlewares/ST/ARM/DSP/Include
        Application
        Application/bsp
        Application/bsp/include
        Application/app
        Application/task
        # 如果 bsp_dwt.h 在具体的子文件夹里，请在这里继续添加，例如：
        # Application/app/algorithm/_imu
)

# 确保包含所有源文件（包括启动汇编文件）
file(GLOB_RECURSE SOURCES
        "Core/*.c"
        "Core/*.cpp"
        "Core/*.s"
        "Drivers/*.c"
        "Application/*.c"
        "Application/*.cpp"
        "USB_DEVICE/*.c"
        "RT-Thread/*.c"
)

# 明确添加 CMSIS-DSP 所需模块（只编译汇总文件，避免重复定义）
# CMSIS-DSP 使用汇总文件架构：每个模块的 *Functions.c 已经 include 了所有单独的 .c 文件
set(DSP_SOURCES
    "Middlewares/ST/ARM/DSP/Source/MatrixFunctions/MatrixFunctions.c"
    "Middlewares/ST/ARM/DSP/Source/BasicMathFunctions/BasicMathFunctions.c"
    "Middlewares/ST/ARM/DSP/Source/FastMathFunctions/FastMathFunctions.c"
    "Middlewares/ST/ARM/DSP/Source/CommonTables/CommonTables.c"
    "Middlewares/ST/ARM/DSP/Source/SupportFunctions/SupportFunctions.c"
)

list(APPEND SOURCES ${DSP_SOURCES})

# 添加 STM32 USB Device 中间件库
file(GLOB_RECURSE USB_MIDDLEWARE_SOURCES "Middlewares/ST/STM32_USB_Device_Library/**/*.c")
list(APPEND SOURCES ${USB_MIDDLEWARE_SOURCES})

# RT-Thread 相关源文件
file(GLOB_RECURSE RT_THREAD_SOURCES "Middlewares/Third_Party/RealThread_RTOS_RT-Thread/*.c")
list(APPEND SOURCES ${RT_THREAD_SOURCES})

# 剔除 main.c (因为通常使用 main.cpp)
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/Core/Src/main.c")
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H723VGTX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-T ${LINKER_SCRIPT})

# --- [关键修复：链接数学库和 DSP 库] ---
add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

# 链接标准数学库（提供 __sqrtf, sinf, cosf 等函数）
# 使用 -Wl,--start-group 和 -Wl,--end-group 确保循环依赖解决
target_link_libraries(${PROJECT_NAME}.elf -Wl,--start-group m c -Wl,--end-group)

# 如果编译报 undefined reference to 'arm_mat_...'，请取消下面两行的注释：
# link_directories(Middlewares/ST/ARM/DSP/Lib/GCC)
# target_link_libraries(${PROJECT_NAME}.elf arm_cortexM7l_math)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}\nBuilding ${BIN_FILE}")