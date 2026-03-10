#!/bin/bash
# build_microros.sh — 构建 micro-ROS 静态库（libmicroros.a）for STM32H723 + GCC
#
# 使用方法（在 WSL / Linux 终端中运行，需安装 Docker）：
#   cd <项目根目录>
#   bash cmake/build_microros.sh
#
# 输出：
#   Middlewares/micro_ros/libmicroros/libmicroros.a
#   Middlewares/micro_ros/libmicroros/microros_include/

set -e

ROS2_DISTRO=humble
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTDIR="$PROJECT_ROOT/Middlewares/micro_ros/libmicroros"

echo "[micro-ROS] Project root: $PROJECT_ROOT"
echo "[micro-ROS] Output dir:   $OUTDIR"
echo "[micro-ROS] ROS 2 distro: $ROS2_DISTRO"

if ! command -v docker &>/dev/null; then
    echo "[ERROR] Docker not found."
    exit 1
fi

docker run --rm \
    -v "$PROJECT_ROOT:/project" \
    --env MICROROS_LIBRARY_FOLDER=Middlewares/micro_ros \
    microros/micro_ros_static_library_builder:$ROS2_DISTRO

# 验证构建产物
if [ -f "$OUTDIR/libmicroros.a" ]; then
    echo ""
    echo "[micro-ROS] Build complete!"
    echo "  Library : $OUTDIR/libmicroros.a ($(du -h "$OUTDIR/libmicroros.a" | cut -f1))"
    echo "  Headers : $OUTDIR/microros_include/"
    echo ""
    echo "Next: rebuild the STM32 project in CLion."
else
    echo ""
    echo "[micro-ROS] ERROR: libmicroros.a not found after build!"
    echo "  Check Docker output above for errors."
    exit 1
fi
