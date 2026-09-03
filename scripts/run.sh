#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════
# 一键启动/恢复可视化环境(幂等:活着的组件不动,只补缺失的)。
#
#   bash scripts/run.sh                  # 确保服务栈 + ds_vis 都在运行
#   bash scripts/run.sh --restart        # 强制重启 ds_vis(重新编译后看新版本)
#   bash scripts/run.sh --restart 密码    # 同时自定义 VNC 密码
#
# 所有进程以 setsid + nohup 启动,SSH 断开不受影响。
# 本地浏览器打开 http://localhost:6080/vnc.html 即可看到界面。
# ═══════════════════════════════════════════════════════════════
set -uo pipefail
cd "$(dirname "$0")/.."

RESTART_APP=0
if [ "${1:-}" = "--restart" ]; then RESTART_APP=1; shift; fi
PASS="${1:-ds2026}"
LOG=/tmp

ensure_svc() {  # $1=pgrep 模式 $2=名称 $3=日志文件名 $4...=启动命令
    local pat="$1" name="$2" log="$3"; shift 3
    if pgrep -f "$pat" > /dev/null 2>&1; then
        echo "[已在运行] $name"
    else
        nohup setsid "$@" > "$LOG/$log" 2>&1 < /dev/null &
        echo "[启动]   $name"
    fi
}

# ── 服务栈:缺哪个补哪个 ──
ensure_svc "Xvfb :1"             "Xvfb :1"          xvfb.log       Xvfb :1 -screen 0 1280x800x24 -ac
sleep 0.5
ensure_svc "x11vnc -display :1"  "x11vnc :5901"     x11vnc.log     x11vnc -display :1 -forever -localhost -passwd "$PASS" -rfbport 5901
ensure_svc "websockify.*6080"    "websockify :6080" websockify.log websockify --web=/usr/share/novnc 6080 localhost:5901

# ── 程序:没跑才拉起;--restart 则先杀再拉 ──
if [ "$RESTART_APP" = 1 ]; then
    if pkill -x ds_vis 2>/dev/null; then
        echo "[停止]   旧 ds_vis"
        sleep 0.5
    fi
fi
if pgrep -x ds_vis > /dev/null 2>&1; then
    echo "[已在运行] ds_vis(重新编译后可加 --restart 换新版本)"
else
    nohup setsid env DISPLAY=:1 SDL_VIDEODRIVER=x11 ./build/ds_vis > "$LOG/ds_vis.log" 2>&1 < /dev/null &
    echo "[启动]   ds_vis(DISPLAY=:1,已脱离终端,断开 SSH 不受影响)"
fi

# ── 健康检查 ──
sleep 1.5
echo ""
FAIL=0
ss -tln 2>/dev/null | grep -q ":6080" || { echo "[异常] 6080 未监听(看 $LOG/websockify.log)"; FAIL=1; }
ss -tln 2>/dev/null | grep -q ":5901" || { echo "[异常] 5901 未监听(看 $LOG/x11vnc.log)"; FAIL=1; }
DISPLAY=:1 timeout 2 xset q > /dev/null 2>&1 || { echo "[异常] 显示器 :1 无响应(看 $LOG/xvfb.log)"; FAIL=1; }
pgrep -x ds_vis > /dev/null 2>&1 || { echo "[异常] ds_vis 未运行(看 $LOG/ds_vis.log)"; FAIL=1; }

if [ "$FAIL" = 0 ]; then
    echo "═══ 全部就绪 ═══"
    echo "  本地浏览器: http://localhost:6080/vnc.html   (VNC 密码: $PASS)"
    echo "  停止环境:   bash scripts/stop_remote_vis.sh"
else
    echo "═══ 有组件异常,按上方提示查日志 ═══"
    exit 1
fi
