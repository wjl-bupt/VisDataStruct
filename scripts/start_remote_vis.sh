#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════
# 启动"远程浏览器可视化"服务栈:
#   Xvfb(:1 虚拟显示器) → x11vnc(抓屏) → websockify/noVNC(浏览器桥)
#
# 之后用 DISPLAY=:1 运行 ./build/ds_vis,即可在浏览器里观看。
# 用户侧(本地电脑)通过 SSH 端口转发访问,见 README「远程浏览器可视化」。
#
# 用法:
#   bash scripts/start_remote_vis.sh              # 默认密码 ds2026
#   bash scripts/start_remote_vis.sh 你的密码      # 自定义 VNC 密码
#   bash scripts/stop_remote_vis.sh               # 停止整套服务
# ═══════════════════════════════════════════════════════════════
set -euo pipefail

VNC_PASS="${1:-ds2026}"
LOGDIR="${TMPDIR:-/tmp}"

# 1) 虚拟显示器 :1(1280x800 与程序窗口一致;-ac 允许本地免鉴权连接)
if pgrep -f "Xvfb :1" > /dev/null 2>&1; then
    echo "[已在运行] Xvfb :1"
else
    echo "[启动] Xvfb :1"
    setsid Xvfb :1 -screen 0 1280x800x24 -ac > "${LOGDIR}/xvfb.log" 2>&1 < /dev/null &
fi
sleep 1

# 2) VNC 抓屏服务(只监听本机回环,由 websockify 桥接;密码保护)
if pgrep -f "x11vnc -display :1" > /dev/null 2>&1; then
    echo "[已在运行] x11vnc (5901)"
else
    echo "[启动] x11vnc (5901, 密码: ${VNC_PASS})"
    setsid x11vnc -display :1 -forever -localhost -passwd "${VNC_PASS}" \
        -rfbport 5901 > "${LOGDIR}/x11vnc.log" 2>&1 < /dev/null &
fi

# 3) noVNC/websockify:把 6080 端口的 HTTP 桥到 VNC
if pgrep -f "websockify.*6080" > /dev/null 2>&1; then
    echo "[已在运行] websockify (6080)"
else
    echo "[启动] websockify (6080 → 5901)"
    setsid websockify --web=/usr/share/novnc 6080 localhost:5901 \
        > "${LOGDIR}/websockify.log" 2>&1 < /dev/null &
fi
sleep 1

echo ""
echo "═══ 服务已就绪 ═══"
echo "  虚拟显示器:  DISPLAY=:1"
echo "  VNC 端口:    5901 (密码: ${VNC_PASS})"
echo "  浏览器入口:  http://localhost:6080/vnc.html"
echo ""
echo "  运行程序:  DISPLAY=:1 ./build/ds_vis"
echo "  日志:      ${LOGDIR}/{xvfb,x11vnc,websockify}.log"
