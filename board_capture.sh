#!/bin/sh
# 捕获开发板原生 X11 终端，不把排版的文本图冒充终端截图。
set -eu
cd "$(dirname "$0")"
export DISPLAY=${DISPLAY:-:0}
export XAUTHORITY=${XAUTHORITY:-/home/cat/.Xauthority}
for stem in 01-create 02-write 03-paused 04-resumed
do
    xterm -T "LubanCat - $stem" -fa Monospace -fs 11 -geometry 102x34+15+35 \
        -bg '#101828' -fg '#E6EDF3' -e sh -c 'cat "$1"; sleep 30' sh "evidence/$stem.log" &
    terminal_pid=$!
    # 字体缓存初始化可能超过两秒，等待窗口映射后再捕获，避免只截到登录页。
    attempts=0
    until xwininfo -root -tree 2>/dev/null | grep -F "LubanCat - $stem" >/dev/null
    do
        attempts=$((attempts + 1))
        if [ "$attempts" -ge 20 ]
        then
            kill "$terminal_pid" 2>/dev/null || true
            printf 'Terminal did not map: %s\n' "$stem" >&2
            exit 1
        fi
        sleep 1
    done
    sleep 2
    if ! xfce4-screenshooter -f -s "$(pwd)/evidence/$stem.png"
    then
        kill "$terminal_pid" 2>/dev/null || true
        exit 1
    fi
    kill "$terminal_pid" 2>/dev/null || true
    wait "$terminal_pid" 2>/dev/null || true
done
