"""Arch-C P3-6：生成免安装 Release zip 骨架。

用法：python tools/make_release.py <构建输出目录，如 x64/Release> [输出 zip 路径]
从构建输出目录收集 exe 与 SFML dll，连同 images/、配置模板、文档打包。
运行前请确保已用 VS 以 Release|x64 构建，并把 dep/SFML/bin 的 dll 放入输出目录。
"""
import os
import sys
import glob
import shutil
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

EXES = ['Server.exe', 'Client.exe', 'Tests.exe']
DOC_FILES = [
    'README.md', 'CURRENT.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md',
    'docs/rules.md', 'docs/characters.md', 'client_config.json', 'server_config.json',
]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    build_dir = os.path.normpath(sys.argv[1])
    if not os.path.isdir(build_dir):
        print(f'构建目录不存在: {build_dir}'); sys.exit(1)
    out_zip = os.path.normpath(sys.argv[2]) if len(sys.argv) > 2 else os.path.join('dist', 'UNOOL-Release.zip')

    found_exes = [e for e in EXES if os.path.exists(os.path.join(build_dir, e))]
    if not found_exes:
        print(f'{build_dir} 未找到任何 exe，请先 Release|x64 构建'); sys.exit(1)
    dlls = glob.glob(os.path.join(build_dir, '*.dll'))

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

    print(f'已生成 {out_zip}')
    print(f'  exe: {", ".join(found_exes)}')
    print(f'  dll: {len(dlls)} 个')
    print('  提示：截图待补（需实际运行游戏截屏后放入 images/screenshots/ 并重新打包）')


if __name__ == '__main__':
    main()
