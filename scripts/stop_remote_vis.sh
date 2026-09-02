#!/usr/bin/env bash
# 停止"远程浏览器可视化"服务栈(Xvfb / x11vnc / websockify)
set -uo pipefail

pkill -f "websockify.*6080" && echo "已停止 websockify" || echo "websockify 未在运行"
pkill -f "x11vnc -display :1" && echo "已停止 x11vnc" || echo "x11vnc 未在运行"
pkill -f "Xvfb :1" && echo "已停止 Xvfb" || echo "Xvfb 未在运行"
