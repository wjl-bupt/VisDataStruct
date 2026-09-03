cmake_minimum_required(VERSION 3.16)
project(ds_visualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # 生成 compile_commands.json,供 IDE 定位头文件

# ── SDL2 ──
# imgui 1.91+ 要求 SDL >= 2.0.17;Ubuntu 20.04 系统包只有 2.0.10,
# 因此使用 third_party/sdl2 里的本地静态构建(见 scripts/build_sdl2.sh)。
# 直接调 pkg-config,绕开各 CMake 版本 FindPkgConfig 对 --static 的支持差异。
set(SDL2_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2")
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env "PKG_CONFIG_PATH=${SDL2_PREFIX}/lib/pkgconfig"
            pkg-config --static --libs sdl2
    OUTPUT_VARIABLE SDL2_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE SDL2_PKG_RESULT)
if(NOT SDL2_PKG_RESULT EQUAL 0)
    message(FATAL_ERROR "未找到本地 SDL2,请先运行: bash scripts/build_sdl2.sh")
endif()
separate_arguments(SDL2_LIBS UNIX_COMMAND "${SDL2_LIBS}")

add_library(sdl2_local INTERFACE)
# 两个路径都挂上:兼容 <SDL.h> 与 <SDL2/SDL.h> 两种写法,且始终优先于系统旧头文件
target_include_directories(sdl2_local INTERFACE
    "${SDL2_PREFIX}/include"
    "${SDL2_PREFIX}/include/SDL2")
target_compile_definitions(sdl2_local INTERFACE _REENTRANT)
target_link_libraries(sdl2_local INTERFACE ${SDL2_LIBS})

# ── imgui(国内网络环境默认走 Gitee 镜像,可用 -DIMGUI_GIT_URL=... 覆盖)──
include(FetchContent)
set(IMGUI_GIT_URL "https://gitee.com/mirrors/imgui.git" CACHE STRING "imgui 仓库地址")
FetchContent_Declare(imgui
    GIT_REPOSITORY ${IMGUI_GIT_URL}
    GIT_TAG v1.91.9b  # 本机 SDL 2.0.10 较老,v1.92+ 需要 SDL ≥ 2.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC sdl2_local)

# ── 主程序 ──
add_executable(ds_vis
    src/main.cpp
    src/scenes/list_scene.cpp
    src/scenes/sort_scene.cpp
    src/scenes/tree_scene.cpp)
target_include_directories(ds_vis PRIVATE src)
target_link_libraries(ds_vis PRIVATE imgui)
target_compile_options(ds_vis PRIVATE -Wall -Wextra)
