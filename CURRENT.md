# CURRENT — 当前状态（唯一现状入口）

> 本文件是仓库现状的单一入口。历史批次日志与旧 ADR 已归档到 [`docs/adr/`](docs/adr/)，
> 其中的「零行为变化 / 线上字节不变 / 逐字平移 / 跨阶段留包语义未动」等验收口径自 Arch-C 起全部作废，
> 不参与现状判断。规则以 [`docs/rules.md`](docs/rules.md) 为准；角色数以 [`docs/characters.md`](docs/characters.md) 为准。

## 代号与分支

- 当前优化代号 **架构 C（Arch-C）**，承接 README 历史「当前架构 B+」的代际命名，是允许破坏性变更的换代。
- 工作分支 `架构C`；提交前缀 `[Arch-C Px-y]`。基线为合并后的 main HEAD（含 `a53e40d` 合并提交）。

## 技术形态

- C++23 / SFML 3.1.0 / nlohmann/json；Visual Studio 2026 `.slnx`，仅 x64，Windows 10/11。
- 4 个工程：`Server`、`Client`（exe）+ `UNOOL.Adapters`、`UNOOL.Core`（静态库）。
- 生成期护栏：`python tools/verify_structure.py --check`（CI 用）、`--generate`（以 manifest 为源重写工程条目）；
  `python tools/gen_stats.py --check`（角色 / 技能统计最新性）。

## 已落地（Arch-C）

| 项 | 状态 | 要点 |
|---|---|---|
| P0-1 工程文件生成器 | 已落地 | `verify_structure.py` 双模式；`tools/projects.manifest.json` 为工程条目唯一事实源；新增 `.cpp` 放进目录即可纳入；覆盖 PCH /FI、BOM、CRLF |
| P0-2 合并 Client | 已落地 | 删除 Client2，保留单一 `Client` 工程；玩家座位由服务端分配 |
| P0-3 清理僵尸 | 已落地 | 移除 Win32 配置与 slnx 的 x86；`ICommands.h` / 占位库 / `reconnect()` / `setCredentials()` 已不存在；`.slnLaunch.user` 已忽略 |
| P1-1 协议版本握手 | 已落地 | `MessageType` 改 `enum class uint16` + 显式分段编号（命令 1xx / 事件 2xx / 心跳 901）；`kProtocolVersion=2`；登录 / 注册包首字段带版本，不匹配返回可读拒绝 |
| P2-1 密码哈希 | 已落地 | salt + PBKDF2-HMAC-SHA256（12 万次，纯标准库实现于 `header/core/Crypto.h`）；落盘无明文；旧明文格式给出重置提示 |
| P2-2 许可证 | 已落地 | 根目录 MIT `LICENSE` + `THIRD_PARTY_NOTICES.md` |
| P2-3 配置外置 | 已落地 | 客户端 IP / 端口 / 存档路径支持命令行与环境变量覆盖；8888 / 14341 默认值不变 |
| P3-1 角色数生成 | 已落地 | `tools/gen_stats.py` 生成 `docs/characters.md` + `tools/generated_stats.json`；当前 123 角色 / 238 技能类 |
| P3-2 README 重组 | 已落地 | 历史批次归档 `docs/adr/README-历史批次.md`，本文件为现状入口 |
| P3-3 规则文档同步 | 已落地 | `docs/rules.md` 增量补充体力/生命上限、摸牌来源、技能触发顺序、积分查表与协议变更说明；一次性归一化为 CRLF |
| P3-4 CI | 已落地 | `.github/workflows/arch-c.yml`：结构/编码/统计校验 + x64 Debug·Release 构建 + 单测运行 |

## 进行中 / 未完成

- **P1-2 序列化收敛**：`Socket.cpp` 仍有约 20+ 处手写包，尚未统一为每种消息一对 `serialize` / `deserialize`。
- **P1-3 收包模型显式化**：`peekMessageType()` 隐式跨阶段留包、迭代器式 `PacketDispatcher`、心跳 / 3 秒超时散落 Scene 与 `run()`，尚未收敛为连接状态机。
- **P1-4 Scene 纯 View 化**：`LoginScene` / `ShopScene` 仍含 `sf::Packet` / `json` 符号；`skillsCache` / `itemPresentationCache` 仍是进程级 static；尚无 fake gateway headless 测试。
- **P3-5 测试网（起点已建，待扩展）**：已有 `Projects/Tests` 零依赖单测工程，23 个断言覆盖 Crypto / 协议版本 / 消息方向 / 积分查表；冻结区（牌堆 / 出牌 / 技能顺序 / 摸牌来源 / 生命上限）与协议 round-trip、两个 Scene 的 headless 测试尚未补。
- **P3-6 Release 产物**：`tools/make_release.py` 免安装打包脚本已就位；实际 zip（需编译后）、运行截图（需实机截屏）、版本 tag 尚未产出。

## 不可丢的玩法语义

- 体力扣减（手牌分值之和）、摸牌来源（`draw_begin` 的 `source`）、生命上限内核支持、按角色等级差查表 `scoreboard`（`header/utils.h`）——重构后必须可对照、可测试。
- 一桌两座位（`contract/TableConfig.h` 的 `seatCount=2`）是玩法规则，不是可开桌数，改动需单独评审。
- 端口 `8888`（服务端监听）与 `14341`（客户端穿透映射）有意不同，保持原值，不加一致性校验。

## 待办口径

- 编译验证：Arch-C 改动尚未在 VS 中做 Debug / Release x64 Rebuild，需在交付后由用户本地确认。
- P1-2 / P1-3 / P1-4 为相互耦合的深度重构，按用户决策暂停，待后续单独排期。
