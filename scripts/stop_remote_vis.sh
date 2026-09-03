#!/usr/bin/env bash
# 完整停止可视化环境:ds_vis 程序 + 服务栈(Xvfb / x11vnc / websockify)
set -uo pipefail

pkill -f "build/ds_vis" && echo "已停止 ds_vis" || echo "ds_vis 未在运行"
pkill -f "websockify.*6080" && echo "已停止 websockify" || echo "websockify 未在运行"
pkill -f "x11vnc -display :1" && echo "已停止 x11vnc" || echo "x11vnc 未在运行"
pkill -f "Xvfb :1" && echo "已停止 Xvfb" || echo "Xvfb 未在运行"
