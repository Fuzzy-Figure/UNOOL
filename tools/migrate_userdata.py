#!/usr/bin/env python3
"""Arch-C P2 账号存档一次性迁移：旧明文口令 -> salt + PBKDF2-HMAC-SHA256。

用法：python tools/migrate_userdata.py <userDatas.json>

先备份为 <文件名>.bak-<时间戳>，再原地改写。已迁移的账号自动跳过，
其余所有字段（积分 / 胜负 / 皮肤 / 道具 / 签到）原样保留，口令不变。

哈希参数与 header/core/Crypto.h + source/core/Crypto.cpp 逐字节一致：
  salt = 16 随机字节 -> 32 位小写十六进制串
  hash = PBKDF2-HMAC-SHA256(口令 UTF-8 字节, salt 十六进制串的 ASCII 字节,
                            120000 次, dkLen=32) -> 64 位小写十六进制串
"""

import hashlib
import json
import secrets
import shutil
import sys
from datetime import datetime
from pathlib import Path

ITERATIONS = 120000
SALT_BYTES = 16


def migrate(path: Path) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("存档根节点必须是账号对象")

    changed = 0
    for username, info in data.items():
        if not isinstance(info, dict):
            raise ValueError(f"账号 {username} 的数据不是对象")
        if "passwordHash" in info and "salt" in info:
            continue
        password = info.get("password")
        if not isinstance(password, str) or not password:
            raise ValueError(f"账号 {username} 缺少可迁移的明文 password")

        salt = secrets.token_hex(SALT_BYTES)
        digest = hashlib.pbkdf2_hmac(
            "sha256",
            password.encode("utf-8"),
            salt.encode("ascii"),
            ITERATIONS,
            dklen=32,
        ).hex()
        info["salt"] = salt
        info["passwordHash"] = digest
        info.pop("password", None)
        changed += 1

    backup = path.with_name(
        path.name + ".bak-" + datetime.now().strftime("%Y%m%d-%H%M%S")
    )
    shutil.copy2(path, backup)
    path.write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(f"已迁移 {changed} 个账号；原文件备份：{backup}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("用法: python tools/migrate_userdata.py <userDatas.json>")
    migrate(Path(sys.argv[1]))
