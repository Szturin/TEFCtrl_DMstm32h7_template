#此文件从模板自动生成! 请勿更改!
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

#Uncomment for hardware floating point
#add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
#add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
#add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
#add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)


# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

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

# CLion 会填充 Core/Inc USB_DEVICE/App USB_DEVICE/Target Drivers/STM32H7xx_HAL_Driver/Inc Drivers/STM32H7xx_HAL_Driver/Inc/Legacy Middlewares/ST/STM32_USB_Device_Library/Core/Inc Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc Drivers/CMSIS/Device/ST/STM32H7xx/Include Drivers/CMSIS/Include Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/include/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/fcntl/msvc/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/fcntl/octal/，我们手动追加 Application 相关路径
include_directories(Core/Inc USB_DEVICE/App USB_DEVICE/Target Drivers/STM32H7xx_HAL_Driver/Inc Drivers/STM32H7xx_HAL_Driver/Inc/Legacy Middlewares/ST/STM32_USB_Device_Library/Core/Inc Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc Drivers/CMSIS/Device/ST/STM32H7xx/Include Drivers/CMSIS/Include Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/include/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/fcntl/msvc/ Middlewares/Third_Party/RealThread_RTOS_RT-Thread/include/libc/extension/fcntl/octal/
        Application
        Application/bsp
        Application/bsp/include
        Application/app
        Application/task
)

add_definitions(-DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx)

# 确保包含 USB_DEVICE 目录
file(GLOB_RECURSE SOURCES
        "Core/*.*"
        "Drivers/*.*"
        "Application/*.*"
        "USB_DEVICE/*.*"   # <--- 必须有这一行
        "Middlewares/*.*"
        "RT-Thread/*.*"
)
# 剔除 main.c
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/Core/Src/main.c")
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H723VGTX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
