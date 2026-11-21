#!/usr/bin/env python3
"""一键启动本地对战：Server + 两个 Client，并尽量错开两个客户端窗口。

用法：
    python tools/start_local.py                  # Debug x64（默认）
    python tools/start_local.py --release        # Release x64
    python tools/start_local.py --port 8888      # 指定端口

客户端用命令行参数 --ip/--port 覆盖配置，因此不会被 client_config.json
里的外网地址 120.226.44.187:14341 带偏。脚本只负责拉起进程，不等待其结束。
"""

import argparse
import ctypes
import socket
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def port_open(ip, port):
    """探测服务是否已在监听，避免重复启动 Server 造成端口冲突。"""
    try:
        with socket.create_connection((ip, port), timeout=0.3):
            return True
    except OSError:
        return False


def arrange_client_windows(pids):
    """尽力把两个客户端主窗口错开；失败不影响游戏运行。"""
    if sys.platform != "win32" or len(pids) < 2:
        return
    try:
        user32 = ctypes.windll.user32
        SWP_NOSIZE = 0x0001
        SWP_NOZORDER = 0x0004
        EnumWindowsProc = ctypes.WINFUNCTYPE(
            ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)

        screen_w = user32.GetSystemMetrics(0)
        targets = set(pids)
        found = {}

        def cb(hwnd, _):
            pid = ctypes.c_ulong()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            if pid.value in targets and user32.IsWindowVisible(hwnd):
                length = user32.GetWindowTextLengthW(hwnd)
                if length > 0:
                    buf = ctypes.create_unicode_buffer(length + 1)
                    user32.GetWindowTextW(hwnd, buf, length + 1)
                    title = buf.value
                    if "Client" in title or "客户端" in title:
                        found.setdefault(pid.value, []).append(hwnd)
            return True

        for _ in range(20):  # 最多等约 6 秒，窗口可能晚于进程出现
            user32.EnumWindows(EnumWindowsProc(cb), 0)
            if all(p in found for p in targets):
                break
            time.sleep(0.3)

        positions = [(20, 20), (min(screen_w // 3, 500), 60)]
        for idx, pid in enumerate(pids):
            for hwnd in found.get(pid, [])[:1]:  # 每个进程只移动主窗口
                x, y = positions[idx % len(positions)]
                user32.SetWindowPos(hwnd, 0, x, y, 0, 0,
                                    SWP_NOSIZE | SWP_NOZORDER)
    except Exception as e:
        print(f"[提示] 窗口错开未完成（不影响运行）：{e}")


def main():
    ap = argparse.ArgumentParser(description="一键启动本地 Server + 两个 Client")
    ap.add_argument("--release", action="store_true", help="使用 Release x64（默认 Debug）")
    ap.add_argument("--ip", default="127.0.0.1", help="服务端 IP（默认 127.0.0.1）")
    ap.add_argument("--port", type=int, default=8888, help="服务端端口（默认 8888）")
    ap.add_argument("--delay", type=float, default=1.5, help="启动 Server 后等待秒数")
    args = ap.parse_args()

    config = "Release" if args.release else "Debug"
    bindir = ROOT / "x64" / config
    server = bindir / "Server.exe"
    client = bindir / "Client.exe"
    missing = [p.name for p in (server, client) if not p.is_file()]
    if missing:
        print(f"[错误] 找不到 {', '.join(missing)}（目录：{bindir}）")
        print(f"请先在 Visual Studio 中以 x64 / {config} 生成解决方案。")
        return 1

    print(f"[本地对战] 配置={config} 服务地址={args.ip}:{args.port}")
    if port_open(args.ip, args.port):
        print("[本地对战] 检测到服务已在监听，跳过启动 Server（复用现有实例）。")
    else:
        srv = subprocess.Popen([str(server)], cwd=str(bindir))
        print(f"[本地对战] 已启动 Server.exe（PID {srv.pid}）")
        time.sleep(args.delay)
        if srv.poll() is not None:
            print("[错误] Server 已退出，请查看服务端控制台或检查端口占用。")
            return 1

    clients = []
    for i in (1, 2):
        c = subprocess.Popen(
            [str(client), "--ip", args.ip, "--port", str(args.port)],
            cwd=str(bindir))
        clients.append(c)
        print(f"[本地对战] 已启动 Client {i}（PID {c.pid}）")

    arrange_client_windows([c.pid for c in clients])
    print("[本地对战] 两个客户端请用不同账号登录（同账号会把对方顶下线）。")
    print("[本地对战] 可用账号：1/1、2/2、Lazer/114、fuzzyfigure/123。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
