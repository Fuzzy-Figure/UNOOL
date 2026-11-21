# CURRENT — 当前状态（唯一现状入口）

> 本文件是仓库现状的单一入口。历史批次日志与旧 ADR 已归档到 [`docs/adr/`](docs/adr/)，
> 其中的「零行为变化 / 线上字节不变 / 逐字平移 / 跨阶段留包语义未动」等验收口径自 Arch-C 起全部作废，
> 不参与现状判断。规则以 [`docs/rules.md`](docs/rules.md) 为准；角色数以 [`docs/characters.md`](docs/characters.md) 为准。

## 代号与分支

- 当前优化代号 **架构 C（Arch-C）**，承接 README 历史「当前架构 B+」的代际命名，是允许破坏性变更的换代。
- 工作分支 `架构C`；提交前缀 `[Arch-C Px-y]`。基线为合并后的 main HEAD（含 `a53e40d` 合并提交）。

## 技术形态

- C++23 / SFML 3.1.0 / nlohmann/json；Visual Studio 2026 `.slnx`，仅 x64，Windows 10/11。
- 5 个工程：`Server`、`Client`、`Tests`（exe）+ `UNOOL.Adapters`、`UNOOL.Core`（静态库）。
- 生成期护栏：`python tools/verify_structure.py --check`（CI 用）、`--generate`（以 manifest 为源重写工程条目）；
  `python tools/gen_stats.py --check`（角色 / 技能统计最新性）。

## 已落地（Arch-C）

| 项 | 状态 | 要点 |
|---|---|---|
| P0-1 工程文件生成器 | 已落地 | `verify_structure.py` 双模式；`tools/projects.manifest.json` 为工程条目唯一事实源；新增 `.cpp` 放进目录即可纳入；覆盖 PCH /FI、BOM、CRLF |
| P0-2 合并 Client | 已落地 | 删除 Client2，保留单一 `Client` 工程；玩家座位由服务端分配；`Projects/Client2/` 构建残留（124 MB）已清除 |
| P0-3 清理僵尸 | 已落地 | 移除 Win32 配置与 slnx 的 x86；`ICommands.h` / 占位库 / `reconnect()` / `setCredentials()` / `PacketDispatcher` + `peekMessageType` 已不存在（含 manifest 与 vcxproj 条目）；`.slnLaunch.user` 已忽略 |
| P1-1 协议版本握手 | 已落地 | `MessageType` 改 `enum class uint16` + 显式分段编号（命令 1xx / 事件 2xx / 心跳 901），**枚举与 `toWire`/`isClientCommand`/`isServerEvent`/`isControlMessage` 均在 `unool::protocol`**；`kProtocolVersion=2`；登录 / 注册包首字段带版本，不匹配返回可读拒绝 |
| P1-2 序列化收敛 | 已落地 | `header/protocol/MessageCodec.h` + `source/protocol/MessageCodec.cpp`：游戏态消息（ConnectionInfo / GameStart / GameEnd / GameState / CharInfo / Choice / PointerUpdate / Heartbeat / ClientInput / ShopPurchase / ShopResult / CheckinResponse）统一成对 make/parse；Socket / ClientApp 手写包全部改走 Codec；Tests 工程加 11 项 round-trip 单测；`header/core` 闭包无第三方头 |
| P1-3 收包状态机 | 已落地 | `header/shell/ConnectionState.h` 单一连接状态机：显式入站队列 + 心跳回复 + 断线清队列；`pump()/take()/takeAny()/contains()` 取代 `peekMessageType` 隐式留包；三处 `net.update()`/`PacketDispatcher` 调用归零；Tests 加 fake gateway 三项单测 |
| P2-1 密码哈希 | 已落地 | salt + PBKDF2-HMAC-SHA256（12 万次，纯标准库实现于 `header/core/Crypto.h`）；落盘无明文；旧明文格式给出重置提示 |
| P2-2 许可证 | 已落地 | 根目录 MIT `LICENSE` + `THIRD_PARTY_NOTICES.md` |
| P2-3 配置外置 | 已落地 | 客户端 IP / 端口 / 存档路径支持命令行与环境变量覆盖；8888 / 14341 默认值不变 |
| P3-1 角色数生成 | 已落地 | `tools/gen_stats.py` 生成 `docs/characters.md` + `tools/generated_stats.json`；当前 123 角色 / 238 技能类 |
| P3-2 README 重组 | 已落地 | 历史批次归档 `docs/adr/README-历史批次.md`，本文件为现状入口 |
| P3-3 规则文档同步 | 已落地 | `docs/rules.md` 增量补充体力/生命上限、摸牌来源、技能触发顺序、积分查表与协议变更说明；一次性归一化为 CRLF |
| P3-4 CI | 已落地 | `.github/workflows/arch-c.yml`：结构/编码/统计校验 + x64 Debug·Release 构建 + 单测运行 |

## 进行中 / 未完成

- **P1-4 Scene 纯 View 化（LoginScene 已完成，ShopScene 待做）**：两个 Scene 内 `sf::Packet` 发包构造 / `jsonStr` 解析 / `skillsCache`+`itemPresentationCache` 进程级 static 已清零——语义收发上移到 `ConnectionState`（sendLogin/sendShopRequest/sendBuyXxx 等 + ShopProtocol Packet 重载），展示缓存抽为组合根持有的 `ShopPresentationCache`。**LoginScene 的三台状态机（Mode / Status / Focus）已整体搬到 `header/shell/LoginViewModel.h`**：纯标准库、无 SFML / 无网络，时间按毫秒注入（3 秒取消兜底可 headless 断言），View 只剩「几何命中 → 语义方法、Actions → ConnectionState/IShopOpener、状态 → 绘制」三件事。剩余：ShopScene 的 `View`/`Mode` 状态机（4 台，含购买确认与技能预览）尚未抽 ViewModel，两个 Scene 仍持 `GameRenderer&`。
- **P3-5 测试网（已覆盖冻结区三项语义，Scene headless 待补）**：`Projects/Tests` 单测工程，23 个纯逻辑断言（Crypto / 协议版本 / 消息方向 / 积分查表）+ P1-2 的 11 组 MessageCodec round-trip + P1-3 的 3 项连接状态机 + `source/tests/test_rules.cpp`（牌值表 / 体力扣减 = 手牌分值之和 / 秘技牌归类与牌池隔离 / 容器与选择指针 / 弃牌元数据随复制保留 / 生命上限夹紧，共 63 项）+ `source/tests/test_gamelogic.cpp`（fake-port 测试台：`draw_begin` 摸牌来源、技能触发顺序、标准牌堆构成 172 张 / 弃牌堆回收 / `canUse` 出牌合法性，26 项）+ `source/tests/test_loginviewmodel.cpp`（P1-4 登录状态机 15 组用例：输入与焦点、登录 / 注册链、发包失败回滚、匹配取消 3 秒兜底、断线与 GameStart 收尾、签到弹窗、匹配横幅、商城后会话判定，共 121 项），合计 210 项。fake-port 测试台在 `source/tests/test_fakes.h`：`IServerView` / `IDecisionPort` / `DomainServices` 全假实现，输入必须预置（未预置的 ask 直接抛异常，不会挂死），加新用例只需「装输入 → 造 GameLogic → 断言」。断言统一走 `source/tests/test_support.h` 的 `TestResult`（**零宏**：`check(ok, name = {})` 用 `std::source_location` 默认实参自带 `file:line`，用例分组用 `section`），六个测试文件共用一份计数与打印。**仍缺**：ShopScene 的 headless 测试（依赖其 ViewModel 切片）。
- **P3-6 Release 产物**：`tools/make_release.py` 免安装打包脚本已就位；实际 zip（需编译后）、运行截图（需实机截屏）、版本 tag 尚未产出。

## 不可丢的玩法语义

- 体力扣减（手牌分值之和）、摸牌来源（`draw_begin` 的 `source`）、生命上限内核支持、按角色等级差查表 `scoreboard`（`header/utils.h`）——重构后必须可对照、可测试。
- 一桌两座位（`contract/TableConfig.h` 的 `seatCount=2`）是玩法规则，不是可开桌数，改动需单独评审。
- 端口 `8888`（服务端监听）与 `14341`（客户端穿透映射）有意不同，保持原值，不加一致性校验。

## 待办口径

- 编译验证：Arch-C 改动尚未在 VS 中做 Debug / Release x64 Rebuild，需在交付后由用户本地确认。
- P1-3 / P1-4 为相互耦合的深度重构，P1-2 已落地，剩余两项待后续排期。
