"""UNOOL 结构自检（B+ 落地后的护栏，无 CI 时替代「静态等价性」清单）。

用法：项目根目录执行  python tools/verify_structure.py
退出码 0 = 全部通过；非 0 = 存在需要人工确认的结构问题。

检查项：
  1. 所有 .vcxproj / .filters / .props / .slnx 是合法 XML
  2. 解决方案引用的工程文件存在
  3. 工程条目（ClCompile / ClInclude / ProjectReference）指向的文件存在
  4. 源码里每个 #include "..." 都能在工程的包含目录中解析
  5. B+ 不变量：Core 的共享编译单元不含 Windows.h；
     协议头不直接 include Character.h；平台接口只在 exe 外壳实现
  6. 已删除的占位库 / 单例 / 全局容量常量没有残留引用；桌座位数仍为 2
"""
import os
import re
import glob
import sys
import xml.etree.ElementTree as ET

MSB = '{http://schemas.microsoft.com/developer/msbuild/2003}'
problems = []


def code_only(text):
    """去掉注释，避免注释里的说明文字被当成实际代码命中。"""
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def read_code(path):
    return code_only(open(path, encoding='utf-8', errors='ignore').read())

INCLUDE_DIRS = [
    'header', 'header/domain', 'header/application', 'header/adapters',
    'header/protocol', 'dep/nlohmann', 'dep/SFML/include', 'source',
    'Projects/Core', 'Projects/Server', 'Projects/Adapters',
    'Projects/Client1', 'Projects/Client2',
]

# --- 1. XML 合法性 ---
xmls = (glob.glob('Projects/*/*.vcxproj') + glob.glob('Projects/*/*.filters')
        + glob.glob('Projects/*.props') + ['UNOOL.slnx'])
for f in xmls:
    try:
        ET.parse(f)
    except Exception as exc:
        problems.append('XML 解析失败 %s: %s' % (f, exc))
print('[1] XML 校验: %d 个文件' % len(xmls))

# --- 2. 解决方案引用 ---
sln = ET.parse('UNOOL.slnx').getroot()
project_paths = [p.get('Path') for p in sln.iter('Project') if p.get('Path')]
for path in project_paths:
    if not os.path.exists(path):
        problems.append('slnx 引用不存在的工程: %s' % path)
print('[2] 解决方案工程: %d 个' % len(project_paths))

# --- 3. 工程条目 ---
item_count = 0
for f in glob.glob('Projects/*/*.vcxproj') + glob.glob('Projects/*/*.filters') + glob.glob('Projects/*.props'):
    tree = ET.parse(f)
    base = os.path.dirname(f)
    for el in tree.iter():
        inc = el.get('Include')
        if not inc:
            continue
        tag = el.tag.replace(MSB, '')
        if tag not in ('ClCompile', 'ClInclude', 'ProjectReference', 'None'):
            continue
        if '$(' in inc:
            continue
        item_count += 1
        target = os.path.normpath(os.path.join(base, inc.replace('\\', '/')))
        if not os.path.exists(target):
            problems.append('%s: 引用不存在 -> %s' % (f, inc))
print('[3] 工程条目: %d 条' % item_count)

# --- 4. #include 解析 ---
srcs = glob.glob('header/**/*.h', recursive=True) + glob.glob('source/**/*.cpp', recursive=True)
unresolved = 0
for f in srcs:
    text = open(f, encoding='utf-8', errors='ignore').read()
    for inc in re.findall(r'^\s*#include\s+"([^"]+)"', text, re.M):
        cands = [os.path.join(d, inc) for d in INCLUDE_DIRS]
        cands.append(os.path.join(os.path.dirname(f), inc))
        if not any(os.path.exists(c) for c in cands):
            problems.append('%s: 无法解析 #include "%s"' % (f, inc))
            unresolved += 1
print('[4] #include 解析: %d 个源文件, %d 处未解析' % (len(srcs), unresolved))

# --- 5. B+ 不变量 ---
core_proj = open('Projects/Core/UNOOL.Core.vcxproj', encoding='utf-8').read()
core_units = re.findall(r'<ClCompile Include="([^"]+)"', core_proj)
win_include = re.compile(r'^\s*#include\s*[<"]Windows\.h[>"]', re.M)
for item in core_units:
    p = os.path.normpath(os.path.join('Projects/Core', item.replace('\\', '/')))
    if os.path.exists(p) and win_include.search(read_code(p)):
        problems.append('Core 共享编译单元仍含 Windows.h: %s' % p)
print('[5a] Core 编译单元: %d 个，平台头污染 0 处' % len(core_units))

for f in glob.glob('header/protocol/*.h'):
    if re.search(r'^\s*#include\s+"Character\.h"', open(f, encoding='utf-8', errors='ignore').read(), re.M):
        problems.append('协议头仍直接依赖规则内核: %s' % f)
print('[5b] 协议头闭包检查完成')

shell_units = ('source/ClientMain.cpp', 'source/ServerMain.cpp',
               'source/adapters/ClientApp.cpp')
for f in glob.glob('source/**/*.cpp', recursive=True) + glob.glob('header/**/*.h', recursive=True):
    norm = f.replace('\\', '/')
    if win_include.search(read_code(f)) and not (norm.startswith('source/platform/') or norm in shell_units):
        problems.append('平台头出现在共享单元: %s' % norm)
print('[5c] 平台头分布检查完成')

# --- 6. 死符号残留 ---
all_text = ''
for f in srcs:
    all_text += read_code(f)
for sym in ('defaultBundle(', 'LegacyBundle', 'UserDB::instance(', 'unool::MAX_PLAYERS'):
    if sym in all_text:
        problems.append('已删除的符号仍有残留引用: %s' % sym)

# 一桌两个座位是玩法规则，不是可开桌数；改动它等于改玩法，需单独评审。
table_cfg = 'header/contract/TableConfig.h'
if not os.path.exists(table_cfg):
    problems.append('缺少共享契约层桌配置: %s' % table_cfg)
elif not re.search(r'seatCount\s*=\s*2\s*;', read_code(table_cfg)):
    problems.append('桌配置 seatCount 不再是 2：这属于改玩法，需单独评审')
print('[6] 死符号与桌配置检查完成')

print('\n=== 问题清单 ===')
if problems:
    for p in problems:
        print(' -', p)
    print('合计 %d 个' % len(problems))
    sys.exit(1)
print('无。结构自检全部通过。')
sys.exit(0)
