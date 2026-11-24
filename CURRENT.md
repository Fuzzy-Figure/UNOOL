# CURRENT — 当前状态（唯一现状入口）

> 本文件是仓库现状的单一入口。历史批次日志与旧 ADR 已归档到 [`docs/adr/`](docs/adr/)，
> 其中的「零行为变化 / 线上字节不变 / 逐字平移 / 跨阶段留包语义未动」等验收口径自 Arch-C 起全部作废，
> 不参与现状判断。规则以 [`docs/rules.md`](docs/rules.md) 为准；角色数以 [`docs/characters.md`](docs/characters.md) 为准。
> Arch-C 的原始工单已归档为 [`docs/adr/UNOOL_优化工单_Arch-C.md`](docs/adr/UNOOL_优化工单_Arch-C.md)（P0~P3 全部落地，仅作追溯）。

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
| P0-1 工程文件生成器 | 已落地 | `verify_structure.py` 双模式；`tools/projects.manifest.json` 为工程条目唯一事实源（**5 个工程全纳入，含 Tests**）；新增 `.cpp` 放进目录即可纳入（测试文件放 `source/tests/` 亦然）；覆盖 PCH /FI、无 PCH 工程的全量 `NotUsing`、BOM、CRLF |
| P0-2 合并 Client | 已落地 | 删除 Client2，保留单一 `Client` 工程；玩家座位由服务端分配；`Projects/Client2/` 构建残留（124 MB）已清除 |
| P0-3 清理僵尸 | 已落地 | 移除 Win32 配置与 slnx 的 x86；`ICommands.h` / 占位库 / `reconnect()` / `setCredentials()` / `PacketDispatcher` + `peekMessageType` 已不存在（含 manifest 与 vcxproj 条目）；`.slnLaunch.user` 已忽略 |
| P1-1 协议版本握手 | 已落地 | `MessageType` 改 `enum class uint16` + 显式分段编号（命令 1xx / 事件 2xx / 心跳 901），**枚举与 `toWire`/`isClientCommand`/`isServerEvent`/`isControlMessage` 均在 `unool::protocol`**；`kProtocolVersion=3`（房间上线后升版）；登录 / 注册包首字段带版本，不匹配返回可读拒绝 |
| P1-2 序列化收敛 | 已落地 | `header/protocol/MessageCodec.h` + `source/protocol/MessageCodec.cpp`：游戏态消息（ConnectionInfo / GameStart / GameEnd / GameState / CharInfo / Choice / PointerUpdate / Heartbeat / ClientInput / ShopPurchase / ShopResult / CheckinResponse）统一成对 make/parse；Socket / ClientApp 手写包全部改走 Codec；Tests 工程加 11 项 round-trip 单测；`header/core` 闭包无第三方头 |
| P1-3 收包状态机 | 已落地 | `header/shell/ConnectionState.h` 单一连接状态机：显式入站队列 + 心跳回复 + 断线清队列；`pump()/take()/takeAny()/contains()` 取代 `peekMessageType` 隐式留包；三处 `net.update()`/`PacketDispatcher` 调用归零；Tests 加 fake gateway 三项单测 |
| P2-1 密码哈希 | 已落地 | salt + PBKDF2-HMAC-SHA256（12 万次，纯标准库实现于 `header/core/Crypto.h`）；落盘无明文；旧明文格式给出重置提示。单次派生 Release ≈46 ms / Debug ≈270 ms（HMAC 密钥块预计算、无堆分配；与优化前逐位等价，含 12 万次的落盘锚点由 `test_main.cpp` 的 RFC 向量锁定）；服务端每次登录/注册会打印这段耗时。**`Crypto.cpp` 单独开 MaxSpeed**（manifest 的 `compile.perFileOptions` → 条目级 `PrecompiledHeader=NotUsing` + `Optimization=MaxSpeed` + `BasicRuntimeChecks=Default`）：`/RTC1` 与优化互斥，Debug 下开着等于强制 `/Od`，实测同一份源码 `/Od+/RTC1` 318 ms、`/O2` 49 ms。**必须同时 `NotUsing` PCH**——`pch.core.h` 是 `/Od+/RTC1` 下生成的，优化选项与之不一致时 cl 只发 C4653 警告并**静默忽略命令行选项**（等于白改，仍在 700 ms）。只影响这一个编译单元，运行时库仍 `/MDd`，无 ODR 风险 |
| P2-2 许可证 | 已落地 | 根目录 MIT `LICENSE` + `THIRD_PARTY_NOTICES.md` |
| P2-3 配置外置 | 已落地 | 客户端 IP / 端口 / 存档路径支持命令行与环境变量覆盖；8888 / 14341 默认值不变 |
| P3-1 角色数生成 | 已落地 | `tools/gen_stats.py` 生成 `docs/characters.md` + `tools/generated_stats.json`，并把 README 顶部「N 名可选角色 / N 个技能类」的两个数字一并回写（`--check` 同时校验三处）；当前 123 角色 / 238 技能类 |
| P3-2 README 重组 | 已落地 | 历史批次归档 `docs/adr/README-历史批次.md`，本文件为现状入口 |
| P3-3 规则文档同步 | 已落地 | `docs/rules.md` 增量补充体力/生命上限、摸牌来源、技能触发顺序、积分查表与协议变更说明；一次性归一化为 CRLF |
| P3-4 CI | 已落地 | `.github/workflows/arch-c.yml`：结构/编码/统计校验 + x64 Debug·Release **矩阵并行**构建，两个配置各跑一遍单测；PR 上同分支新 run 顶掉旧 run |
| P1-4 Scene 纯 View 化 | 已落地 | 两个 Scene 内 `sf::Packet` 发包构造 / `jsonStr` 解析 / 进程级 static 缓存清零；语义收发上移 `ConnectionState`，展示缓存抽为组合根持有的 `ShopPresentationCache`；状态机在 `LoginViewModel` / `ShopViewModel`（纯标准库、无 SFML / 无网络、时间按毫秒注入），出站走 `takeActions()` 队列；View 只剩几何命中、Actions 执行与绘制 |
| P3-5 测试网 | 已落地 | `Projects/Tests` 单测工程，9 个测试文件覆盖冻结区玩法语义、两个 ViewModel、协议全量 round-trip（含 32 个消息号方向分段清单）、UserDB 存档与结算；fake-port 测试台在 `source/tests/test_fakes.h`，断言器零宏（`TestResult` + `std::source_location`） |
| 房间功能（协议版本 3） | 已落地 | 登录后不再匹配：主大厅（开始游戏 / 商城 / 每日签到）→ 点「开始游戏」进房间界面（列表 + 创建 / 加入 / 刷新 / 返回大厅）→ 房内双方准备 → 自动开局。**服务端多房间真并发**：`ServerNetwork`（`header/shell/ServerNetwork.h`）是唯一 socket 持有者与唯一 IO 线程，按槽位管理连接、出站队列与心跳；`RoomManager` 管房间生命周期（房主转让 / 空房解散 / 对局中禁止进出）；`RoomGame` 每房一份 `GameLogic` + 一个线程，`RoomServerView` / `RoomDecisionAdapter` 把「座位 0/1 ↔ 连接槽位」映射好，房间线程只碰输入队列与出站队列，不并发访问 socket。`GameStart` 带本局座位号；共享端口加锁（`JsonUserRepository` / `Mt19937Random` / `JsonConfigProvider`）。Windows 与安卓客户端同步落地房间界面。同账号在另一端登录时后登录端顶替：旧连接关闭前先收 `SessionReplaced`（221）通知，客户端提示「你已在别处登录，本端被顶下线」并退回登录界面。 |

## 进行中 / 未完成

代码侧无剩余项。以下三件各自有明确前置条件：

- **P3-6 剩余两件（需本机实机操作）**：`tools/make_release.py` 免安装打包脚本已就位，并已用 `x64/Release` 产物实际跑通一次（386 条目 / 118.5 MB：Server+Client、5 个 SFML dll、369 个 `images/` 资源、配置模板与文档，外加包内生成的「启动游戏.bat」与「快速开始.txt」，解开双击即可开局；`Tests.exe` 不进玩家包，`dist/` 不进版本库）。待补：运行截图（实机截屏后放 `images/screenshots/`，重跑脚本即入包）、版本 tag（待 PR 合回 main 后打）。
- **已知限制（改则是 UI 行为变更，非缺陷）**：商城搜索模式下结果超过一页时翻不到第 2 页——A/D 已被关键词输入占用，View 的搜索分支只绑了 ↑↓ / Enter / Backspace / Esc。要支持需给 ←/→ 单独绑翻页。
- **P1-4 有意不外抽的部分**：两个 Scene 仍持 `GameRenderer&`，渲染依赖属 View 层本职，不再抽离。

## 测试网索引

`Projects/Tests` 单测工程，断言散布在 9 个测试文件：`test_main.cpp`（Crypto / 协议版本 / 消息方向 / 积分查表）、`test_codec.cpp`（P1-2 MessageCodec 成对 round-trip，含 `GameStart` 座位号）、`test_protocol.cpp`（账号协议登录/注册/签到、房间命令与房间事件的成对 round-trip、商城 JSON 的成对 round-trip + 32 个消息号的方向分段清单，新增枚举值漏分段会直接失败）、`test_connection.cpp`（P1-3 入站队列 / 心跳 / 断线 / 被顶下线通知）、`test_rules.cpp`（牌值表 / 体力扣减 = 手牌分值之和 / 秘技牌归类与牌池隔离 / 容器与选择指针 / 弃牌元数据 / 生命上限夹紧）、`test_gamelogic.cpp`（fake-port 测试台：`draw_begin` 摸牌来源、技能触发顺序、标准牌堆 172 张 / 弃牌回收 / `canUse`）、`test_loginviewmodel.cpp`（P1-4 登录状态机 + 房间流程 19 组用例：输入规则、注册链、开始游戏进入房间界面 / 返回大厅、房间列表刷新、创建/加入/离开房间、房内准备与开局、断线结束会话）、`test_shopviewmodel.cpp`（P1-4 商城状态机 14 组用例 / 151 项：分页与展开、搜索过滤与 UTF-8 退格、购买判定与动作、提示消息 2.5 秒边界、技能请求去重与缓存命中、空数据不崩）、`test_userdb.cpp`（P2-1/P2-3 账号存档 9 组用例：注册口令哈希落盘无明文、登录校验、旧明文存档按旧格式拒绝、积分增减与余额不足、角色/皮肤/道具购买与消耗、重开进程式磁盘往返、等级差结算与满血/皮肤加成、签到当日去重、集齐档位解锁 S 档；存档路径用 `UNOOL_USERDATA` 指到系统临时目录，不碰仓库 `userDatas.json`）。fake-port 测试台在 `source/tests/test_fakes.h`：`IServerView` / `IDecisionPort` / `DomainServices` 全假实现，输入必须预置（未预置的 ask 直接抛异常，不会挂死），加新用例只需「装输入 → 造 GameLogic → 断言」。断言统一走 `source/tests/test_support.h` 的 `TestResult`（**零宏**：`check(ok, name = {})` 用 `std::source_location` 默认实参自带 `file:line`，用例分组用 `section`），九个测试文件共用一份计数与打印。

## 不可丢的玩法语义

- 体力扣减（手牌分值之和）、摸牌来源（`draw_begin` 的 `source`）、生命上限内核支持、按角色等级差查表 `scoreboard`（`header/utils.h`）——重构后必须可对照、可测试。
- 一桌两座位（`contract/TableConfig.h` 的 `seatCount=2`）是玩法规则；进程能开几桌由
  `server_config.json` 的 `network.maxClients`（默认 32 → 16 个房间）与 `RoomManager` 决定，
  两者互不影响。桌内的对手判定（`1 - seat`）只在座位域内成立，不要跨槽位套用。
- 端口 `8888`（服务端监听）与 `14341`（客户端穿透映射）有意不同，保持原值，不加一致性校验。

## 待办口径

- 编译验证：Arch-C 全部改动已在 VS 中 x64 Debug / Release 编译通过，Tests 单测全绿。
- P0-1 ~ P3-5 全部落地；P1-3 / P1-4 的相互耦合重构已完成。P3-6 只剩用户侧两件（截图、tag）。
- **房间功能（协议版本 3）待用户 VS 编译验证**：服务端换成「IO 线程 + 每房一线程」，
  客户端大厅 / 房间界面与安卓房间界面均为新增；工程条目已用
  `python tools/verify_structure.py --generate` 按 manifest 重写（新增 4 个 .cpp / 4 个头，
  删除 `LegacyBlockingDecisionAdapter.*` 与 `NetworkServerView.*`）。
