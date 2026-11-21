"""UNOOL 结构生成器 / 校验器（Arch-C P0-1）。

两种模式：
  python tools/verify_structure.py --generate   依据 tools/projects.manifest.json 重写
                                                5 个 .vcxproj 与 .filters 的条目区
  python tools/verify_structure.py --check      校验工程条目与 manifest 一致 + 结构不变量
                                                （默认模式，CI 使用）

条目区由标记注释界定：
  <!-- ARCH-C:ITEMS:BEGIN --> ... <!-- ARCH-C:ITEMS:END -->
标记之外的一切（配置、PCH 使用、链接库、Import、ProjectReference）均由本脚本保留不动。

manifest 是工程文件条目的唯一事实源：
  - compile.recursiveDirs 下的 .cpp 自动纳入（新增 .cpp 放进这些目录即可）
  - compile.files / headers.files 是共享目录（source/shell）与显式需要的清单
  - 头文件、None、PCH、Filter 分类都由 manifest 推导
  - pch 缺省 / "mode": "none" 表示该工程不用 PCH（Tests 全量 NotUsing）

保留的既有结构检查（原 B+ 护栏）：
  1. XML 合法  2. slnx 引用  3. 条目指向的文件存在  4. #include 可解析
  5. Core 不含 Windows.h  6. 死符号与桌座位数  7. B+ 语义命名
新增（Arch-C）：
  8. 工程条目与 manifest 派生结果一致（--check 核心）
  9. BOM / CRLF / UTF-8 编码约定
"""
import os
import re
import glob
import json
import sys
import argparse
import uuid
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

MSB = '{http://schemas.microsoft.com/developer/msbuild/2003}'
MANIFEST = 'tools/projects.manifest.json'
BEGIN = '<!-- ARCH-C:ITEMS:BEGIN -->'
END = '<!-- ARCH-C:ITEMS:END -->'
problems = []

# Filter 名 -> 固定 GUID（沿用 VS 默认与既有稳定值），未知项用确定性 uuid5 生成
FILTER_GUIDS = {
    '源文件': '4FC737F1-C7A5-4376-A066-2A32D752A2FF',
    '头文件': '93995380-89BD-4b04-88EB-625FBE52EBFB',
    '资源文件': '67DA6AB6-F800-4c08-8B7A-83BB121AAD01',
    '头文件\\core': '2563460C-23A4-56F8-86D1-60638978DDB8',
    '头文件\\shell': 'BF12D6D3-E121-5CD9-848D-219AF17DE8D0',
    '头文件\\protocol': '57718106-80DB-5D3C-AEFD-7351FA66CF0C',
    '头文件\\contract': '46F2EE96-CBF9-507F-980D-6A75DE38C118',
    '头文件\\platform': '9525211D-22E0-5EC7-B1C4-A264DE45F4C5',
    '源文件\\core': '397E7E3E-ACCE-59D3-94F5-57C0E2252EF5',
    '源文件\\shell': '585B3392-20E9-5081-A23F-B19395B8E7CE',
    '源文件\\protocol': '40355DB1-53E7-5C71-AE44-0AD2608D73B0',
}
FILTER_NS = uuid.UUID('6c93f1e4-2a1b-4d0e-9f77-1e7c0a4b8d21')


def filter_guid(name):
    if name in FILTER_GUIDS:
        return FILTER_GUIDS[name]
    return str(uuid.uuid5(FILTER_NS, name)).upper()


def norm(p):
    return p.replace('\\', '/')


def rel_to(proj_path, target):
    """返回从工程目录到目标的 Windows 相对路径（target 为仓库根相对）。"""
    proj_dir = os.path.abspath(os.path.dirname(proj_path))
    tgt = os.path.abspath(target)
    rel = os.path.relpath(tgt, proj_dir)
    return rel.replace('/', '\\')


def read_text(path):
    with open(path, encoding='utf-8-sig', newline='') as fh:
        return fh.read()


def write_text(path, text, bom):
    data = text.replace('\r\n', '\n').replace('\r', '\n').replace('\n', '\r\n')
    enc = 'utf-8-sig' if bom else 'utf-8'
    with open(path, 'w', encoding=enc, newline='') as fh:
        fh.write(data)


def has_bom(path):
    with open(path, 'rb') as fh:
        return fh.read(3) == b'\xef\xbb\xbf'


def code_only(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def read_code(path):
    return code_only(read_text(path))


def load_manifest():
    with open(MANIFEST, encoding='utf-8') as fh:
        return json.load(fh)


def expand_compile(pr):
    """返回该工程 .cpp 的绝对路径列表（去重、排序）。"""
    found = set()
    for d in pr['compile'].get('recursiveDirs', []):
        for f in glob.glob(d + '/**/*.cpp', recursive=True):
            found.add(norm(f))
    for f in pr['compile'].get('files', []):
        found.add(norm(f))
    # 只有声明了 pch.create 的工程才有 PCH 创建单元；Tests 这类无 PCH 工程跳过
    pch_src = (pr.get('pch') or {}).get('create')
    if pch_src:
        found.add(norm(pch_src))
    # 平台外壳由 PlatformShell.props 提供给 exe，不进任何工程条目
    return sorted(found)


def expand_headers(pr):
    found = set()
    for d in pr['headers'].get('recursiveDirs', []):
        for f in glob.glob(d + '/**/*.h', recursive=True):
            found.add(norm(f))
    for f in pr['headers'].get('files', []):
        found.add(norm(f))
    # PCH 头已在各工程 headers.files 中显式登记（带仓库根相对路径）
    return sorted(found)


def cpp_item_block(rel, pch_create, pch_header):
    if pch_create is None:
        # 无 PCH 工程（Tests）：每个单元显式 NotUsing，避免继承到默认的 Use。
        return ('    <ClCompile Include="%s">\n'
                '      <PrecompiledHeader>NotUsing</PrecompiledHeader>\n'
                '    </ClCompile>' % rel)
    if norm(rel) == norm(pch_create):
        return ('    <ClCompile Include="%s">\n'
                '      <PrecompiledHeader>Create</PrecompiledHeader>\n'
                '      <PrecompiledHeaderFile>%s</PrecompiledHeaderFile>\n'
                '    </ClCompile>' % (rel, pch_header))
    return ('    <ClCompile Include="%s">\n'
            '      <PrecompiledHeader>Use</PrecompiledHeader>\n'
            '      <PrecompiledHeaderFile>%s</PrecompiledHeaderFile>\n'
            '      <ForcedIncludeFiles>%s;%%(ForcedIncludeFiles)</ForcedIncludeFiles>\n'
            '    </ClCompile>' % (rel, pch_header, pch_header))


def build_items_xml(pr, m):
    """生成托管区正文：ClCompile / ClInclude / None 三个 ItemGroup。"""
    proj_path = pr['vcxproj']
    # pch.create 是仓库根相对路径，需转成工程相对再与 cpp_rels 比较，
    # 否则 PCH 创建文件会被误标成 Use 导致 C1854 / C1083。
    pch = pr.get('pch') or {}
    pch_create = rel_to(proj_path, pch['create']) if pch.get('create') else None
    pch_header = pch.get('header', '')

    cpp_rels = [rel_to(proj_path, f) for f in expand_compile(pr)]
    h_rels = [rel_to(proj_path, f) for f in expand_headers(pr)]
    none_rels = [rel_to(proj_path, f) for f in pr.get('none', [])]

    lines = []
    lines.append('  <ItemGroup>')
    for rel in cpp_rels:
        lines.append(cpp_item_block(rel, pch_create, pch_header))
    lines.append('  </ItemGroup>')
    lines.append('  <ItemGroup>')
    for rel in h_rels:
        lines.append('    <ClInclude Include="%s" />' % rel)
    lines.append('  </ItemGroup>')
    if none_rels:
        lines.append('  <ItemGroup>')
        for rel in none_rels:
            lines.append('    <None Include="%s" />' % rel)
        lines.append('  </ItemGroup>')
    return '\n'.join(lines)


def manage_region(text, body):
    """把 BEGIN/END 之间的内容替换为 body；若标记不存在则首次插入。"""
    block = BEGIN + '\n' + body + '\n' + END
    if BEGIN in text and END in text:
        return re.sub(re.escape(BEGIN) + r'.*?' + re.escape(END), lambda _: block, text, flags=re.S)
    # 首次运行：删除旧的顶层 ClCompile/ClInclude/None ItemGroup，
    # 在第一个 ProjectConfiguration ItemGroup 之前插入托管区。
    text = re.sub(r'\s*<ItemGroup>\s*<ClCompile Include=.*?</ItemGroup>', '', text, flags=re.S)
    text = re.sub(r'\s*<ItemGroup>\s*<ClInclude Include=.*?</ItemGroup>', '', text, flags=re.S)
    text = re.sub(r'\s*<ItemGroup>\s*<None Include=.*?</ItemGroup>', '', text, flags=re.S)
    anchor = '  <ItemGroup Label="ProjectConfigurations">'
    if anchor not in text:
        raise RuntimeError('找不到插入锚点：' + anchor)
    return text.replace(anchor, block + '\n' + anchor, 1)


# ---------- filters 生成 ----------
def filter_for_file(rel):
    """按工程相对路径推导 Filter 名。"""
    p = norm(rel)
    if p.endswith('.json'):
        return '资源文件'
    parts = p.split('/')
    is_cpp = p.endswith('.cpp')
    root = '源文件' if is_cpp else '头文件'
    # 形如 ..\..\source\core\skills\元首\Passive.cpp
    if 'source' in parts:
        i = parts.index('source')
        segs = parts[i + 1:-1]
    elif 'header' in parts:
        i = parts.index('header')
        segs = parts[i + 1:-1]
    else:  # 工程内 pch
        segs = []
    if not segs:
        return root
    return root + '\\' + '\\'.join(segs)


def build_filters_xml(pr, m):
    proj_path = pr['vcxproj']
    cpp_rels = [rel_to(proj_path, f) for f in expand_compile(pr)]
    h_rels = [rel_to(proj_path, f) for f in expand_headers(pr)]
    none_rels = [rel_to(proj_path, f) for f in pr.get('none', [])]

    groups = []
    for kind, rels in (('ClCompile', cpp_rels), ('ClInclude', h_rels), ('None', none_rels)):
        for rel in rels:
            f = filter_for_file(rel)
            groups.append('    <%s Include="%s">\n      <Filter>%s</Filter>\n    </%s>' % (kind, rel, f, kind))

    # 收集所有需要的 Filter（含祖先），按层级排序
    needed = set()
    for _, rels in (('c', cpp_rels), ('h', h_rels), ('n', none_rels)):
        for rel in rels:
            f = filter_for_file(rel)
            segs = f.split('\\')
            for k in range(1, len(segs) + 1):
                needed.add('\\'.join(segs[:k]))

    header = ('<?xml version="1.0" encoding="utf-8"?>\n'
              '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\n'
              '  <ItemGroup>')
    filters_def = []
    for name in sorted(needed):
        filters_def.append('    <Filter Include="%s">\n'
                           '      <UniqueIdentifier>{%s}</UniqueIdentifier>\n'
                           '    </Filter>' % (name, filter_guid(name)))

    out = [header]
    out.append('\n'.join(filters_def))
    out.append('  </ItemGroup>')
    out.append('  <ItemGroup>')
    out.append('\n'.join(groups))
    # props 提供的编译单元（PlatformShell.props 里的 WindowsPlatform.cpp）。
    # 它们不进 vcxproj 条目（由 Import 带入），但 VS 会在 .filters 里补一行
    # 以便在解决方案资源管理器里显示；这里照写一遍，避免 VS 保存后
    # --check 报「与 manifest 派生结果不一致」而反复来回改。
    for entry in pr.get('propsCompile', []):
        out.append('    <ClCompile Include="%s" />' % entry)
    out.append('  </ItemGroup>')
    out.append('</Project>')
    return '\n'.join(out)


def do_generate(m):
    print('=== 生成模式：以 manifest 为事实源 ===')
    for pr in m['projects']:
        vp = pr['vcxproj']
        body = build_items_xml(pr, m)
        text = read_text(vp)
        new = manage_region(text, body)
        write_text(vp, new, has_bom(vp))
        print('  已更新条目:', vp)

        fp = pr['filters']
        filters_body = build_filters_xml(pr, m)
        write_text(fp, filters_body, has_bom(fp))
        print('  已更新筛选:', fp)
    print('生成完成。请随后用 --check 复核。')


def do_check(m):
    print('=== 校验模式 ===')
    # --- 8. 工程条目与 manifest 一致 ---
    for pr in m['projects']:
        vp = pr['vcxproj']
        text = read_text(vp)
        if BEGIN not in text or END not in text:
            problems.append('%s: 缺少托管区标记，请先运行 --generate' % vp)
            continue
        mm = re.search(re.escape(BEGIN) + r'\s*\n(.*?)\n\s*' + re.escape(END), text, re.S)
        if not mm:
            problems.append('%s: 托管区标记格式异常' % vp)
            continue
        actual = mm.group(1)
        expect = build_items_xml(pr, m)
        a = [x.strip() for x in actual.splitlines() if x.strip()]
        e = [x.strip() for x in expect.splitlines() if x.strip()]
        cpp_rels_expect = [rel_to(vp, f) for f in expand_compile(pr)]
        if a != e:
            missing = [x for x in e if x not in a]
            extra = [x for x in a if x not in e]
            for x in missing[:8]:
                problems.append('%s: 缺少条目 -> %s' % (vp, x))
            for x in extra[:8]:
                problems.append('%s: 多余条目 -> %s' % (vp, x))
            if len(missing) > 8 or len(extra) > 8:
                problems.append('%s: 另有 %d 缺 / %d 多' % (vp, max(0, len(missing) - 8), max(0, len(extra) - 8)))
        # PCH 语义护栏：manifest 声明的创建单元必须恰好被标为 Create。
        # 防止生成器把 pch.create 误判为普通 .cpp（曾导致 C1854 / C1083）。
        pch = pr.get('pch') or {}
        creates = [i for i, ln in enumerate(e) if ln == '<PrecompiledHeader>Create</PrecompiledHeader>']
        if pch.get('create'):
            pch_rel = rel_to(vp, pch['create'])
            create_line = '<ClCompile Include="%s">' % pch_rel
            if create_line in e:
                idx = e.index(create_line)
                if idx + 1 >= len(e) or e[idx + 1] != '<PrecompiledHeader>Create</PrecompiledHeader>':
                    problems.append('%s: PCH 创建单元 %s 未标记为 Create（会导致 C1854/C1083）' % (vp, pch_rel))
            if len(creates) != 1:
                problems.append('%s: PCH Create 单元数量为 %d（应为 1）' % (vp, len(creates)))
        else:
            # 无 PCH 工程：不允许出现 Create，且每个编译单元都必须是 NotUsing。
            if creates:
                problems.append('%s: 未声明 PCH 却出现 Create 单元（%d 处）' % (vp, len(creates)))
            not_using = sum(1 for ln in e if ln == '<PrecompiledHeader>NotUsing</PrecompiledHeader>')
            if not_using != len(cpp_rels_expect):
                problems.append('%s: NotUsing 单元 %d 个，与 .cpp 数 %d 不符'
                                % (vp, not_using, len(cpp_rels_expect)))
        # filters 一致性
        fp = pr['filters']
        fa = [x.strip() for x in read_text(fp).splitlines() if x.strip()]
        fe = [x.strip() for x in build_filters_xml(pr, m).splitlines() if x.strip()]
        if fa != fe:
            problems.append('%s: 与 manifest 派生结果不一致，请运行 --generate' % fp)
    print('[8] 工程条目/manifest 一致性检查完成')

    # --- 9. 编码 / 行尾约定 ---
    cfg = m['conventions']
    vcxprojs = [pr['vcxproj'] for pr in m['projects']]
    filters = [pr['filters'] for pr in m['projects']]
    for f in vcxprojs:
        if has_bom(f) and not cfg['vcxprojBom']:
            problems.append('vcxproj 不应带 BOM: %s' % f)
    for f in filters:
        if not has_bom(f) and cfg['filtersBom']:
            problems.append('filters 应带 BOM: %s' % f)
    for f in vcxprojs + filters:
        b = open(f, 'rb').read()
        if b.count(b'\n') != b.count(b'\r\n'):
            problems.append('非 CRLF 行尾: %s' % f)
    print('[9] 编码 / 行尾约定检查完成')


def do_legacy_checks():
    """保留原有 B+ 结构护栏。"""
    xmls = (glob.glob('Projects/*/*.vcxproj') + glob.glob('Projects/*/*.filters')
            + glob.glob('Projects/*.props') + ['UNOOL.slnx'])
    for f in xmls:
        try:
            ET.parse(f)
        except Exception as exc:
            problems.append('XML 解析失败 %s: %s' % (f, exc))
    print('[1] XML 校验: %d 个文件' % len(xmls))

    sln = ET.parse('UNOOL.slnx').getroot()
    project_paths = [p.get('Path') for p in sln.iter('Project') if p.get('Path')]
    for path in project_paths:
        if not os.path.exists(path):
            problems.append('slnx 引用不存在的工程: %s' % path)
    print('[2] 解决方案工程: %d 个' % len(project_paths))

    item_count = 0
    for f in xmls:
        if f == 'UNOOL.slnx':
            continue
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

    INCLUDE_DIRS = [
        'header', 'header/core', 'header/shell', 'header/protocol',
        'dep/nlohmann', 'dep/SFML/include', 'source',
        'Projects/Core', 'Projects/Server', 'Projects/Adapters', 'Projects/Client',
    ]
    srcs = glob.glob('header/**/*.h', recursive=True) + glob.glob('source/**/*.cpp', recursive=True)
    unresolved = 0
    for f in srcs:
        text = read_text(f)
        for inc in re.findall(r'^\s*#include\s+"([^"]+)"', text, re.M):
            cands = [os.path.join(d, inc) for d in INCLUDE_DIRS]
            cands.append(os.path.join(os.path.dirname(f), inc))
            if not any(os.path.exists(c) for c in cands):
                problems.append('%s: 无法解析 #include "%s"' % (f, inc))
                unresolved += 1
    print('[4] #include 解析: %d 个源文件, %d 处未解析' % (len(srcs), unresolved))

    core_proj = read_text('Projects/Core/UNOOL.Core.vcxproj')
    core_units = re.findall(r'<ClCompile Include="([^"]+)"', core_proj)
    win_include = re.compile(r'^\s*#include\s*[<"]Windows\.h[>"]', re.M)
    for item in core_units:
        p = os.path.normpath(os.path.join('Projects/Core', item.replace('\\', '/')))
        if os.path.exists(p) and win_include.search(read_code(p)):
            problems.append('Core 共享编译单元仍含 Windows.h: %s' % p)
    print('[5a] Core 编译单元: %d 个' % len(core_units))

    for f in glob.glob('header/protocol/*.h'):
        if re.search(r'^\s*#include\s+"Character\.h"', read_text(f), re.M):
            problems.append('协议头仍直接依赖规则内核: %s' % f)
    print('[5b] 协议头闭包检查完成')

    # 平台头只允许出现在 source/platform/ 与两个 exe 入口；shell 层一律走 platform:: 接口。
    shell_units = ('source/ClientMain.cpp', 'source/ServerMain.cpp')
    for f in srcs:
        norm = f.replace('\\', '/')
        if win_include.search(read_code(f)) and not (norm.startswith('source/platform/') or norm in shell_units):
            problems.append('平台头出现在共享单元: %s' % norm)
    print('[5c] 平台头分布检查完成')

    all_text = ''.join(read_code(f) for f in srcs)
    for sym in ('defaultBundle(', 'LegacyBundle', 'UserDB::instance(', 'unool::MAX_PLAYERS'):
        if sym in all_text:
            problems.append('已删除的符号仍有残留引用: %s' % sym)
    table_cfg = 'header/contract/TableConfig.h'
    if not os.path.exists(table_cfg):
        problems.append('缺少共享契约层桌配置: %s' % table_cfg)
    elif not re.search(r'seatCount\s*=\s*2\s*;', read_code(table_cfg)):
        problems.append('桌配置 seatCount 不再是 2：这属于改玩法，需单独评审')
    print('[6] 死符号与桌配置检查完成')

    OLD_DIRS = ('header/domain', 'header/application', 'header/adapters',
                'source/domain', 'source/application', 'source/adapters')
    for d in OLD_DIRS:
        if os.path.isdir(d):
            problems.append('旧分层目录仍存在: %s' % d)
    NEW_DIRS = ('header/core', 'header/shell', 'header/contract',
                'source/core', 'source/shell', 'source/platform')
    for d in NEW_DIRS:
        if not os.path.isdir(d):
            problems.append('B+ 目录缺失: %s' % d)
    short_ns = re.compile(r'(?<![:\w])(?:domain|application|adapters)::')
    for f in srcs:
        if short_ns.search(read_code(f)):
            problems.append('%s: 旧命名空间简写残留' % f)
    print('[7] B+ 语义命名不变量检查完成')


def main():
    ap = argparse.ArgumentParser(description='UNOOL 结构生成器 / 校验器')
    ap.add_argument('--generate', action='store_true', help='以 manifest 为源重写工程条目')
    ap.add_argument('--check', action='store_true', help='校验模式（默认）')
    args = ap.parse_args()
    m = load_manifest()
    if args.generate:
        do_generate(m)
        return
    do_check(m)
    do_legacy_checks()
    print('\n=== 问题清单 ===')
    if problems:
        for p in problems:
            print(' -', p)
        print('合计 %d 个' % len(problems))
        sys.exit(1)
    print('无。结构校验全部通过。')


if __name__ == '__main__':
    main()
