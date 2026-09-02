#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════
# 构建 SDL2 到项目本地目录 third_party/sdl2(静态库)。
#
# 为什么需要:imgui 1.91+ 的 SDL2 渲染后端要求 SDL >= 2.0.17,
# 而 Ubuntu 20.04 的系统包只有 2.0.10。此脚本把新版 SDL2 编译进
# 项目目录,不打乱系统环境,最终链接进二进制(静态),运行零依赖。
#
# 用法: bash scripts/build_sdl2.sh   (一次即可,构建系统自动使用)
# ═══════════════════════════════════════════════════════════════
set -euo pipefail

SDL_VERSION="2.30.9"
PREFIX="$(cd "$(dirname "$0")/.." && pwd)/third_party/sdl2"
BUILD_DIR="$(mktemp -d)"

echo "==> 下载 SDL2-${SDL_VERSION} 源码 (libsdl.org)"
curl -fsSL -o "${BUILD_DIR}/sdl.tar.gz" "https://www.libsdl.org/release/SDL2-${SDL_VERSION}.tar.gz"

echo "==> 解压并配置 (prefix=${PREFIX})"
tar xzf "${BUILD_DIR}/sdl.tar.gz" -C "${BUILD_DIR}"
cd "${BUILD_DIR}/SDL2-${SDL_VERSION}"
./configure --prefix="${PREFIX}" \
    --disable-shared --enable-static \
    --disable-audio \
    > /dev/null

echo "==> 编译 (make -j)"
make -j"$(nproc)" > /dev/null

echo "==> 安装到 ${PREFIX}"
make install > /dev/null

echo "==> 完成:${PREFIX}/lib/pkgconfig/sdl2.pc"
rm -rf "${BUILD_DIR}"
