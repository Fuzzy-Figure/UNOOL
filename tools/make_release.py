"""Arch-C P3-6：生成免安装 Release zip。

用法：python tools/make_release.py <构建输出目录，如 x64/Release> [输出 zip 路径]
从构建输出目录收集 exe 与 SFML dll，连同 images/、配置模板、文档打包，
并在包内生成「启动游戏.bat」与「快速开始.txt」——包解开即可双击开局，
不需要 Python、不需要 VS2026。

运行前请确保已用 VS 以 Release|x64 构建（dll 由工程复制到输出目录）。
"""
import glob
import os
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Tests.exe 是开发期单测可执行文件，不进玩家包。
EXES = ['Server.exe', 'Client.exe']
# SFML 5 个 dll 一个都不能少，否则双击客户端会以 0xc0000135 直接退出。
REQUIRED_DLLS = [
    'sfml-system-3.dll', 'sfml-window-3.dll', 'sfml-graphics-3.dll',
    'sfml-network-3.dll', 'sfml-audio-3.dll',
]
DOC_FILES = [
    'README.md', 'CURRENT.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md',
    'docs/rules.md', 'docs/characters.md', 'client_config.json', 'server_config.json',
]

# 包内 exe 全在根目录（不像仓库里在 x64/<Config>/），所以不能用
# 一键启动本地对战.bat + tools/start_local.py 那套（它按 x64/Debug|Release 找程序）。
LAUNCHER_BAT = '\r\n'.join([
    '@echo off',
    'chcp 65001 >nul',
    'cd /d "%~dp0"',
    'echo [UNOOL] 正在启动本地服务端...',
    'start "" Server.exe',
    'timeout /t 2 /nobreak >nul',
    'echo [UNOOL] 正在启动两个客户端...',
    'start "" Client.exe --ip 127.0.0.1 --port 8888',
    'start "" Client.exe --ip 127.0.0.1 --port 8888',
    'echo [UNOOL] 请分别用账号 1 / 1 与 2 / 2 登录（同账号会把对方顶下线）',
    'echo [UNOOL] 服务端窗口不要关，关掉即断开对局',
    'pause',
    '',
])

QUICKSTART_TXT = '\r\n'.join([
    'UNOOL 免安装试玩包',
    '',
    '1. 双击「启动游戏.bat」：脚本会拉起一个服务端 + 两个客户端。',
    '2. 两个客户端分别用账号 1 / 1 与 2 / 2 登录（同账号会把对方顶下线）。',
    '3. 登录后进入大厅，双方都点「准备」即开始一局。',
    '',
    '单开客户端（连局域网里的朋友）：',
    '    Client.exe --ip <对方 IP> --port 8888',
    '',
    '只起服务端：双击 Server.exe（默认监听 8888，端口可用 server_config.json 改）。',
    '',
    '目录说明：',
    '    images/         卡面 / 角色立绘等资源，删掉会花屏',
    '    client_config.json / server_config.json   连接与端口配置模板',
    '    docs/rules.md   规则说明；docs/characters.md  角色一览',
    '',
    '常见问题：',
    '    * 双击 exe 提示缺少 dll：确认 5 个 sfml-*-3.dll 与 exe 在同一目录。',
    '    * 客户端连不上：先确认服务端窗口已经打印监听日志，再检查端口占用。',
    '',
])

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    build_dir = os.path.normpath(sys.argv[1])
    if not os.path.isdir(build_dir):
        print(f'构建目录不存在: {build_dir}')
        sys.exit(1)
    out_zip = os.path.normpath(sys.argv[2]) if len(sys.argv) > 2 else os.path.join('dist', 'UNOOL-Release.zip')

    found_exes = [e for e in EXES if os.path.exists(os.path.join(build_dir, e))]
    if len(found_exes) != len(EXES):
        missing = [e for e in EXES if e not in found_exes]
        print(f'{build_dir} 缺少 {", ".join(missing)}，请先 Release|x64 构建')
        sys.exit(1)
    dlls = glob.glob(os.path.join(build_dir, '*.dll'))
    missing_dlls = [d for d in REQUIRED_DLLS if not os.path.exists(os.path.join(build_dir, d))]
    if missing_dlls:
        print(f'警告：{build_dir} 缺少 {", ".join(missing_dlls)}，包里会少了运行库')
    if not os.path.isdir('images'):
        print('警告：仓库缺少 images/ 目录，打出来的包会花屏')

    os.makedirs(os.path.dirname(out_zip) or '.', exist_ok=True)
    with zipfile.ZipFile(out_zip, 'w', zipfile.ZIP_DEFLATED) as z:
        for e in found_exes:
            z.write(os.path.join(build_dir, e), e)
        for d in dlls:
            z.write(d, os.path.basename(d))
        for f in DOC_FILES:
            if os.path.exists(f):
                z.write(f, f)
        for root, _, files in os.walk('images'):
            for name in files:
                p = os.path.join(root, name)
                z.write(p, p.replace('\\', '/'))
        z.writestr('启动游戏.bat', LAUNCHER_BAT.encode('utf-8'))
        z.writestr('快速开始.txt', QUICKSTART_TXT.encode('utf-8'))

    size_mb = os.path.getsize(out_zip) / (1024 * 1024)
    print(f'已生成 {out_zip}（{size_mb:.1f} MB）')
    print(f'  exe: {", ".join(found_exes)}')
    print(f'  dll: {len(dlls)} 个（必需 {len(REQUIRED_DLLS)} 个）')
    print('  包内：启动游戏.bat / 快速开始.txt / images/ / 规则与配置模板')
    print('  提示：截图待补（实机截屏后放入 images/screenshots/ 重新打包）')


if __name__ == '__main__':
    main()
