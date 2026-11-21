# UNOOL 优化工单 · 架构 C / Arch-C（交给执行 agent）

## 0. 上下文

- 仓库：`https://github.com/Fuzzy-Figure/UNOOL`；「新角色」分支已合并进 `main`。本次优化代号 **架构 C（Arch-C）**——承接 README「当前架构（B+）」的代际命名，表示这不是阶段 4 的又一批补丁，而是一次允许破坏性变更的换代
- 工作分支：**`arch-c`**（从 `main` 切出）。所有 P0~P3 改动都提交到该分支，`main` 只作为最终 PR 合并目标；不要直接向 `main` 提交，也不要再 checkout 已合并的「新角色」分支
- 基线：合并前的最新提交为 `472ccd8`（2026-11-15，新增荒野乱斗角色：波克、佩佩、邦尼，含摸牌来源与生命上限内核支持），合并前共 464 commits。**开工第一步 `git checkout main && git pull && git checkout -b arch-c`，以切出时的 `main` HEAD 为实际基线**；`472ccd8` 仅用于标识内容来源，若合并产生了新的合并提交，不要按该哈希 checkout
- 提交前缀：统一用 `[Arch-C P0-1]` 这类格式，便于在 PR 里按优先级筛选与逐项追溯
- 项目形态：C++23 / SFML 3.1.0 / nlohmann/json，VS2026 `.slnx` + 5 个 `.vcxproj`（Server / Client1 / Client2 + Adapters / Core），仅 x64，Windows 10/11
- 现状要点：B+ 架构为「轻量规则内核 + 单一共享契约 + 少量真实接缝」；Core 静态库统一编译 20 个 `.cpp`；规则主体 179 个技能类、96 个角色条目为冻结区；阶段 4 已完成 S1（`cfa91f3`）/ S2（`6f9e71b`）/ S3a（`d8699a2`），S3b1 已删除 `reconnect()`，S3b2 / S3c 的协议上移与两个 Scene 纯 View 化尚未落地
- 本工单基于仓库 README 自述整理，**落地前请先按源码复核每一条的现状**，已在源码中不存在的项直接标记为「无需处理」并在提交信息里说明，不要重复实现
- **已有 `docs/rules.md` 记录游戏规则**，是规则描述的唯一事实源；本次只做增量复核与同步，不要新建同名文件、不要覆盖重写

## 1. 约束（最高优先级，先读这一节）

1. **允许破坏性变更**：修改代码可以接受线上包字节改变、协议与旧版不兼容、客户端与服务端必须同版本升级。README 与 `docs/` 里阶段 0~4 各批次的「零行为变化」「线上字节不变」「逐字平移」「跨阶段留包语义一律未动」等历史验收口径，**自本次优化起全部作废，不得作为设计约束**。历史上为保字节而做的技术妥协（迭代器式分发、手写包、单枚举 25 值、不改函数体）现在都可以推翻。
2. **端口不是缺陷**：`server_config.json` / 客户端配置里的 `8888` 与 `14341` 不一致，是内网穿透映射造成的部署拓扑差异，属预期设计。**不要统一端口、不要加端口一致性校验、不要修改端口相关逻辑、不要在文档里把它列为问题**。允许补充一行注释说明二者分别用于内网监听与穿透映射。
3. **玩法语义不能丢**：体力扣减、摸牌来源、最新提交新增的「生命上限内核支持」、按双方角色等级差查表的积分规则，重构后必须可对照、可测试；允许改实现、改协议、改数据格式，但不允许静默丢规则。
4. **历史分层口径已归档**：六边形分层（Adapters → Application → Domain）、Q8 纯标准库 Domain PCH、Domain / Application 占位库、「目录名不代表架构层」等自 B+ 起仅作归档，不要为维持这些口径保留死代码或占位工程。
5. **节奏**：按 §2 的 P0 → P1 → P2 → P3 顺序推进，每一项单独提交，提交信息注明优化代号与工单编号（如 `[Arch-C P0-1] ...`，见 §0）。每项完成后跑一遍当时的构建与测试再进下一项；全部完成后由 `arch-c` 提 PR 合回 `main`。

## 2. 优化项

### P0 · 先止住每批必现的同步漏项

**P0-1 工程文件生成器（最高优先）**
现状：每次新增 `.cpp` 都要手工在 3 个 exe 的 `ClCompile` 同步登记、再同步 `.vcxproj.filters`；S1 / S2 / S3a 各批都把「双工程 ClCompile 一致」「Include 路径 0 缺失」列为静态验证项，说明这是反复出现的人力开销。
动作：把 `tools/verify_structure.py` 升级为「校验 + 生成」双模式：按 `source/` / `header/` 的目录归属扫描文件，自动生成 5 个 `.vcxproj` 的 `ClCompile` / `ClInclude` 与 `.filters` 条目，并在 CI 里以 `--check` 模式阻止漂移。生成器需覆盖 `PlatformShell.props` 的 3 个 exe 共用清单、`pch.core.h` / `pch.h` 的 `/FI` 设置、UTF-8/CRLF 约定。
验收：删掉任意一个 `ClCompile` 条目后跑生成器能自动补回；新增一个 `.cpp` 只需放进目录、不改任何工程文件即可通过 `--check` 与本地构建；各批次静态验证里出现的「5 / 7 / 10 个工程」三种说法以生成器输出为唯一口径。

**P0-2 合并 Client1 / Client2**
现状：两个工程共用同一份 `ClientMain.cpp`，仅 `ProjectGuid` 不同。
动作：删掉重复工程，保留单一 `Client` 工程；玩家编号、服务端地址、端口从 `client_config.json` 或命令行参数读取，支持同时启动两个实例且配置互不覆盖。
验收：一条命令起两个客户端可完成一局对局；解决方案里不再存在两份内容近乎相同的 `.vcxproj` / `.filters`。

**P0-3 清理僵尸配置与死代码**
动作：
- 删除 Win32 两条配置（README 自承「项目实际只构建 x64，Win32 仅保持包含目录一致」），或显式标为 Unsupported 并从默认构建矩阵移除
- 确认 `UNOOL.slnLaunch.user` 已被 `.gitignore` 忽略；用户级启动配置不进版本库
- 删除零引用骨架：`ICommands.h`、`setCredentials()`（README 记录「全项目零调用点，按 46.7 ③B 不擅改、仅记录」，现予授权），以及其它经全局搜索确认无引用的空壳类 / 空壳方法
- 若仍存在只编译 1 个 `pch.cpp`、0 个业务单元的 Domain / Application 占位库，直接删除并清理 `ProjectReference`；若源码中已删除，确认无残留引用即可
- 确认 `ClientNetwork::reconnect()` 一次性收包循环已按第二十一批 S3b1 口径删除；如仍在，按不可达代码删除
验收：全仓库无「零引用」告警项；`git grep` 找不到上述符号；x64 Debug / Release 均构建通过。

### P1 · 协议与收包模型（可放手重做）

**P1-1 协议版本握手与消息号重分组**
现状：`MessageType` 是 25 值单枚举，未拆 `ClientCommand` / `ServerEvent`；无版本协商。
动作：新增显式 `protocol version` 握手字段，版本不匹配时服务端明确拒绝并返回可读错误（而不是解析出乱码或崩溃）；把 25 个消息号按方向拆成两组并重新编号，改为 `enum class` 显式赋值。既然允许不兼容旧版，不必保留旧数值。
验收：旧客户端连新服务端被明确拒绝并给出版本不匹配提示；新客户端之间全消息号互通。

**P1-2 序列化收敛**
现状：`AccountProtocol` helper、`ReadyToPlay` / `CheckinRequest` / `CheckinResponse` 手写包、`ShopScene` 的 3 处 `json::parse` + 3 处 `get<T>()` + 4 处 `dump()` 混用；`ShopData.h` 因含 `<json.hpp>` 与序列化宏被从 `domain/` 归位到 `protocol/`。
动作：统一到一套 DTO 序列化方案（`sf::Packet` 或 nlohmann/json 二选一，全项目一致），删掉手写包与散落的 parse/dump；每种消息给出成对的 `serialize` / `deserialize`；DTO 头不再依赖第三方头外泄到规则内核。
验收：每个消息号一个 round-trip 单测；`header/core/` 闭包内不出现 `<json.hpp>` 或 SFML 网络头。

**P1-3 收包模型显式化**
现状：`peekMessageType()` 承担「非账号包不消费、留在队列给 `gamePhase`」的隐式跨阶段留包约定；S2 为压住改动面选了迭代器式 `PacketDispatcher::next()`，把复杂度留在运行时；`next(ClientNetwork&)` 绑具体类型（README 自列「障碍 3」）；心跳回复散在三处，3 秒超时与断线 / 关窗判定散在 Scene 与 `run()`。
动作：
- 用带 tag 的显式入站队列或订阅回调取代「peek 后不消费」约定，跨阶段留包由队列语义保证，不再依赖调用顺序
- `PacketDispatcher` 泛化到入站端口接口，去掉对 `ClientNetwork` 具体类型的绑定
- 心跳回复、3 秒超时、断线 / 关窗收敛到单一连接状态机；`run()` 与 Scene 不再各自维护网络时序
验收：Scene 与主循环里 grep 不到 `peekMessageType` / `net.update()` / 心跳分支；跨阶段留包顺序有专门测试用例覆盖；断线场景行为可枚举。

**P1-4 补齐结果回传通道，收官 S3b / S3c**
现状：`IClientSession` 是纯标准库 6 方法端口，`IClientGateway`（S3a）只补了 `send` / `sendShopPurchase` / `setPlayerId`，不含 `receivePacket` / `peekMessageType`；S3b 前置评估列的四个卡点中，入站能力、结果回传通道、`run()` 每帧时序、Dispatcher 具体类型仍需收敛；`ShopScene::skillsCache` 是进程级静态业务缓存，归属未定；两个 Scene 合计约 1591 行，含 6 个 `enum class` 约 50 处枚举分支。
动作：在 P1-3 的入站端口基础上补齐结果回传通道，把 `LoginScene` / `ShopScene` 纯 View 化；`skillsCache` 移到组合根持有的显式缓存对象；`LoginScene.cpp:150` 商城嵌套 S3a 已等价上移，此处收尾；6 个状态机保留但由 View 模型驱动，改成可 headless 测试。
验收：`LoginScene` / `ShopScene` 目录里 grep 不到 `Socket`、`ClientNetwork`、SFML Network、`json` 符号；两个 Scene 可脱离网络用 fake gateway 跑状态机测试。

### P2 · 账号、安全、分发

**P2-1 密码不再明文**
现状：`userDatas.json` 密码明文，仅靠 `.gitignore` 兜底。
动作：改为哈希 + 每账号随机盐（argon2id / scrypt，或至少 PBKDF2-HMAC-SHA256）；明文旧数据不兼容可接受，给出「重置账号数据」或一次性迁移路径并在 README 写明。
验收：落盘文件中不含任何明文密码；注册 / 登录 / 查重 / 积分消费回归通过。

**P2-2 LICENSE 与第三方许可证清单**
动作：仓库根补 `LICENSE`；新增 `THIRD_PARTY_NOTICES.md`，逐项列出 `dep/` 内置依赖的许可证：SFML 3.1.0（zlib/libpng）及其外部库（freetype、libogg / libvorbis / libflac、HarfBuzz、SheenBidi、cpp-unicodelib、Mbed TLS、libssh2、stb、miniaudio、qoi 等），以及 nlohmann/json。补项目自身 LICENSE 与补第三方清单是两件事，都要做。

**P2-3 配置外置**
动作：`client_config.json` 里的服务端 IP / 穿透端口、存档路径支持环境变量或命令行覆盖，不硬编码内网穿透地址；`server_config.json` 的候选数、手牌数、禁用角色保持可配置并补字段说明。`8888` / `14341` 保持原值（见 §1 第 2 条）。

### P3 · 文档、测试与可交付

**P3-1 单一事实源**
现状：README 开头「70+ 名可选角色」，架构段「96 个角色条目 / 179 个技能类」，靠「具体数量与完整列表以代码为准」打补丁。
动作：构建期从 `Character::infos` 自动生成角色数 / 技能数与角色表，注入 README 或生成 `docs/characters.md`；README 只引用生成结果。

**P3-2 README 重组 + 历史归档**
动作：README 顶部只留 简介、玩法规则一句话概述（链接指向已有的 `docs/rules.md`）、构建与运行、截图、自动生成的角色数；规则细节不重复写进 README。把阶段 0~4 二十余批日志整包移入 `docs/adr/`，加归档横幅与状态台账（已落地 / 已废弃 / 被本次优化取代）；新增 `CURRENT.md` 作为唯一现状入口；删掉满篇「目录名不代表架构层」「下文 domain/application 均为历史路径」这类补丁式解释。历史文件保留但不参与现状判断。

**P3-3 规则文档同步（已有文件，禁止重建）**
现状：`docs/rules.md` 已存在并记录游戏规则，不要新建、不要整体重写、不要清空后重排。
动作：先完整读一遍现有 `docs/rules.md`，只做增量：
- 核对 P0~P2 改动后是否仍与实现一致（体力扣减、摸牌来源、最新提交新增的「生命上限内核支持」语义、角色等级差积分查表、技能触发顺序）；若某条本次实现有变更，同步更新对应段落
- 只补现有文档缺失的项，已写清楚的段落原样保留
- 与 P3-5 的内核单测同源维护：文档里的每条判定规则都能对应到一个测试用例
- 因协议与账号数据格式已破坏性变更，若文档涉及存档 / 登录 / 商城交互描述，一并校正
验收：`docs/rules.md` 提交历史为增量 diff（非整文件替换）；无重复章节、无与 `docs/characters.md`（若生成）互相矛盾的数值。

**P3-4 CI**
现状：README 自承「`tools/verify_structure.py` 结构自检（无 CI 时的自动护栏）」，多批构建依赖用户手动 Debug / Release x64 Rebuild，回归清单一度积压 7 批。
动作：补 GitHub Actions，跑 `verify_structure.py --check`、x64 Debug / Release 构建、单元测试、`git diff --check`、Include 路径 / 裸指针检查、编码与行尾检查。
包体 golden fixtures 仍建议保留，但用途改为「检测本次是否有意为之的协议变更」：允许有意识地更新 fixtures，并在 PR 描述里记录变更内容，不再用于保证字节不变。

**P3-5 测试网**
动作：给冻结区补单测——牌堆与出牌合法性、技能触发顺序、摸牌来源、生命上限、积分查表；协议 round-trip 覆盖全部消息号；两个 Scene 用 fake gateway 做 headless 状态机测试。目标是把「179 个技能类 + 96 个角色条目零改动」从「没人敢动」变成「有人兜底」。

**P3-6 Release 产物**
动作：出免安装 zip（含所需 dll、`images/`、配置模板、快速开始 README），补几张实际截图；「新角色」已合并进 main、本次也无分支改名需求，待 `arch-c` 经 PR 合回 `main` 后，在 main 的合并提交上打版本 tag 并发布 Release，降低 0 star / 需自装 VS2026 才能试玩的门槛。

## 3. 明确不要做的事

- 不要统一或修改 `8888` / `14341`，不要把它写成待修问题
- 不要再为「线上字节不变」「零行为变化」「逐字平移」「跨阶段留包语义一律未动」做设计妥协
- 不要保留仅为历史口径存在的占位库、死代码、零引用方法（除非源码证明仍被引用）
- 不要继续往 README 追加阶段批次日志
- 不要再 checkout 已合并的「新角色」分支、不要基于它开工或提 PR；基线一律是 main
- 不要直接向 `main` 提交优化改动；所有提交落在 `arch-c`，完成后以 PR 合回 main
- 不要新建或整体重写已有的 `docs/rules.md`，也不要把规则细节复制回 README
- 不要静默丢弃体力 / 摸牌来源 / 生命上限 / 积分查表等玩法语义

## 4. 完成定义（提交前逐项勾选）

- [ ] 新增 `.cpp` 不需要手改任何 `.vcxproj` / `.filters`
- [ ] 只存在一个 Client 工程，可同时起两个实例
- [ ] Win32 配置、`.slnLaunch.user`、`ICommands.h`、`setCredentials()` 等僵尸项已清理或显式标注
- [ ] 有协议版本握手，旧版连接被明确拒绝
- [ ] 每种消息一组 serialize / deserialize + round-trip 单测
- [ ] `peekMessageType` 隐式留包约定已移除，连接状态机单点持有心跳与超时
- [ ] 两个 Scene 网络符号清零，可 headless 测试
- [ ] `userDatas.json` 无明文密码；`LICENSE` 与 `THIRD_PARTY_NOTICES.md` 齐备
- [ ] 角色数 / 技能数由代码生成，README 无自相矛盾数字
- [ ] `CURRENT.md` 就位、已有 `docs/rules.md` 经增量复核并随本次改动同步，历史批次已归档
- [ ] CI 全流程跑通，冻结区有单测覆盖
- [ ] 免安装 zip 与截图发布
- [ ] `8888` / `14341` 保持原值，未新增端口一致性校验
- [ ] 全部 P0~P3 提交落在 `arch-c`、统一 `[Arch-C Px-y]` 前缀，经 PR 合回 `main`
