"""Arch-C P3-1：从源码生成角色/技能统计与角色表。

唯一事实源：source/core/Character.cpp 的 Character::infos + header/core/skills/**/*.h。
产出（生成物，勿手改）：
  docs/characters.md          角色表（名字 / 分组 / 等级 / 技能）
  tools/generated_stats.json  机器可读统计（README / CI 引用）

用法：python tools/gen_stats.py [--check]
  --check 模式下只比对现有产物是否最新（CI 用）。
"""
import os
import re
import json
import glob
import sys
import argparse

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

CHARACTER_CPP = 'source/core/Character.cpp'
OUT_MD = 'docs/characters.md'
OUT_JSON = 'tools/generated_stats.json'

LEVEL_ORDER = ['S', 'A', 'B', 'C', 'D', 'F']


def parse_infos():
    text = open(CHARACTER_CPP, encoding='utf-8').read()
    start = text.index('Character::infos = {')
    body = text[start:text.index('\n};', start)]
    rows = []
    # 每条: {"名字", {"分组", Level::X, {技能列表}, hp}}
    pat = re.compile(
        r'\{\s*"([^"]+)"\s*,\s*\{\s*"([^"]+)"\s*,\s*Level::(\w+)\s*,\s*\{([^}]*)\}'
        r'(?:\s*,\s*(\d+))?\s*\}', re.S)
    for m in pat.finditer(body):
        name, group, level, skills_raw, hp = m.groups()
        skills = re.findall(r'(被动|即时|转换)<\s*([^>]+?)\s*>', skills_raw)
        skill_list = [{'kind': k, 'name': n.strip()} for k, n in skills]
        rows.append({
            'name': name,
            'group': group,
            'level': level,
            'hp': int(hp) if hp else None,
            'skills': skill_list,
        })
    return rows


def count_skill_classes():
    """统计技能类定义数（class Xxx : public ... 在 skills 分组头中）。"""
    total = 0
    by_group = {}
    for f in glob.glob('header/core/skills/*/*.h'):
        group = os.path.basename(os.path.dirname(f))
        text = open(f, encoding='utf-8').read()
        n = len(re.findall(r'\bclass\s+\w+\s*(?:final\s*)?:\s*public\b', text))
        by_group[group] = by_group.get(group, 0) + n
        total += n
    return total, dict(sorted(by_group.items()))


def build_stats(rows):
    skill_classes, by_group_skills = count_skill_classes()
    by_level = {}
    by_group = {}
    skill_refs = set()
    for r in rows:
        by_level[r['level']] = by_level.get(r['level'], 0) + 1
        by_group[r['group']] = by_group.get(r['group'], 0) + 1
        for s in r['skills']:
            skill_refs.add((r['group'], s['name']))
    return {
        'characterCount': len(rows),
        'skillClassCount': skill_classes,
        'skillClassByGroup': by_group_skills,
        'skillReferenceCount': len(skill_refs),
        'characterByLevel': {k: by_level[k] for k in LEVEL_ORDER if k in by_level},
        'characterByGroup': dict(sorted(by_group.items())),
    }


def build_md(rows, stats):
    lines = [
        '# 角色表（自动生成）',
        '',
        '> 本文件由 `tools/gen_stats.py` 从 `source/core/Character.cpp` 生成，请勿手工编辑。',
        '> 角色数 / 技能数为构建期统计结果，README 只引用本文件的数字。',
        '',
        f"- 角色条目数：**{stats['characterCount']}**",
        f"- 技能类数：**{stats['skillClassCount']}**",
        f"- 角色引用的技能数（按分组去重）：**{stats['skillReferenceCount']}**",
        '',
        '## 按等级分布',
        '',
        '| 等级 | 角色数 |',
        '|---|---:|',
    ]
    for level, n in stats['characterByLevel'].items():
        lines.append(f'| {level} | {n} |')
    lines += ['', '## 按分组分布', '', '| 分组 | 角色数 | 技能类数 |', '|---|---:|---:|']
    skill_by_group = stats['skillClassByGroup']
    for group, n in stats['characterByGroup'].items():
        lines.append(f'| {group} | {n} | {skill_by_group.get(group, 0)} |')
    lines += ['', '## 完整角色表', '', '| 角色 | 分组 | 等级 | 生命 | 技能 |', '|---|---|---|---:|---|']
    for r in rows:
        skills = '、'.join(f"{s['kind']}·{s['name']}" for s in r['skills']) or '—'
        hp = r['hp'] if r['hp'] is not None else '—'
        lines.append(f"| {r['name']} | {r['group']} | {r['level']} | {hp} | {skills} |")
    lines.append('')
    return '\n'.join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()

    rows = parse_infos()
    stats = build_stats(rows)
    md = build_md(rows, stats)
    js = json.dumps(stats, ensure_ascii=False, indent=2) + '\n'

    if args.check:
        ok = True
        if not os.path.exists(OUT_MD) or open(OUT_MD, encoding='utf-8').read() != md:
            print('characters.md 不是最新，请运行 python tools/gen_stats.py'); ok = False
        if not os.path.exists(OUT_JSON) or open(OUT_JSON, encoding='utf-8').read() != js:
            print('generated_stats.json 不是最新，请运行 python tools/gen_stats.py'); ok = False
        if ok:
            print(f'角色/技能统计为最新：{stats["characterCount"]} 角色 / {stats["skillClassCount"]} 技能类')
        sys.exit(0 if ok else 1)

    os.makedirs('docs', exist_ok=True)
    # 与仓库 CRLF 约定一致（--check 读取时 universal newline 会归一化为 \n 后比较）。
    open(OUT_MD, 'w', encoding='utf-8', newline='\r\n').write(md)
    open(OUT_JSON, 'w', encoding='utf-8', newline='\r\n').write(js)
    print(f'已生成 {OUT_MD} 与 {OUT_JSON}')
    print(f'角色 {stats["characterCount"]} / 技能类 {stats["skillClassCount"]} / 技能引用 {stats["skillReferenceCount"]}')


if __name__ == '__main__':
    main()
