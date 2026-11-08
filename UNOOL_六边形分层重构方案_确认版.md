# UNOOL 六边形分层重构方案 · 确认版（第一步）

> 原方案：`C:\Users\Admin\Desktop\UNOOL_六边形分层重构方案.md`
> 确认日期：2026-11-08。本文件是原方案的**确认修订版与本次执行记录**。
> 未在下文中列为"调整/排除"的原方案条款保持不变；阶段 2–6 **本次未执行**，仅作为后续路线。

## 1. 确认决策

| 编号 | 结论 |
|---|---|
| Q1 | 本次只做「落地第一步」的最小可编译版本 |
| Q2 | 不删 `Client2` 工程、不删 Win32 配置 |
| Q3 | 端口不一致、config 注释、`MessageType` 拆分、长度帧/Partial、子技能异常、安全修复全部排除，之后单独处理。已确认端口不一致是内网穿透的有意配置，**不是缺陷、不得修改** |
| Q4 | 接受验收降级：编译 + 静态依赖扫描 + 人工回归清单，不建黄金回放设施 |
| Q5 | 接受 A3/A4/A5：领域保留循环骨架、端口只提供原子能力、谓词留领域、`GameLogic` 保留同名兼容门面 |
| Q6 | 确认后的方案版本在执行后随代码一起交付（本文件） |
| Q7 | 不物理移动现有文件；新工程命名 `UNOOL.Domain/Application/Adapters`（`Protocol` 命名约定保留）；新类型命名空间 `unool::domain` 等；`using player_id = std::size_t` |
| Q8 | 每个静态库各自一份 PCH，绝不共享；`Domain` 单独建纯标准库 PCH |
| Q9 | 商城 `ShopScene`/`LoginScene`/`ClientMain` 的 TODO 暂不纳入 |
| Q10 | 不要求新旧客户端混跑策略（但本次仍严格保持协议字节不变） |
| Q11 | 允许用本机 MSBuild 编译验证 —— 实际被环境安全策略拦截，见第 7 节 |

## 2. 相对原方案的调整（原因 / 影响）

| # | 调整 | 原因 | 影响 |
|---|---|---|---|
| A1 | 范围收敛为「基线只读冻结 + §10 第一步」，不一次建齐 6 个工程、不迁移现有文件 | 完整阶段 0–4 涉及约 1.7 万行且协议/工程/领域同时变动，无法与"零行为变化"同时保证 | 交付可编译的分层边界；阶段 2–4 留后续 |
| A2 | 剔除端口统一、config 注释清理、`MessageType` 拆分、长度帧/Partial 修复、子技能耦合修复、安全修复、删除 Client2/Win32、阶段 5/6 | 均为行为变化或删除，违反本次硬约束 | 已知限制原样保留 |
| A3 | "ask/chooseCard 循环整体搬进适配器"改为"领域保留循环骨架，端口只提供原子 I/O 能力" | 循环内含 `collectAvailableSkills`、`tryActivate`、`tryUseBombCard`、`OperatingGuard`、`broadcastState` 等领域调用，适配器无法承载 | 领域仍含少量 SFML 类型；技能文件零改动 |
| A4 | `CardQuery`/`PlayerQuery` 不承载 `std::function`；端口只回传原始输入/索引，谓词判定留领域 | 现有 `condition` 是捕获任意状态的 lambda，值类型无法表达 | 端口签名与原文 3.2 不同，以本版为准 |
| A5 | 保留 `GameLogic` 同名兼容门面，去掉 `ServerNetwork&` 成员，状态出口改走 `IServerView` | 原文 3.3 与 ② 对 `GameLogic` 去留表述冲突；4400+ 行技能签名依赖 `GameLogic&` | 技能实现零改动；Domain 净化留阶段 3 |
| A6 | 保持 `header/`+`source/` 平铺，仅用 vcxproj 列表做逻辑分组；`header/pch.h` 不动 | 移动文件会波及三个 vcxproj 的相对路径与包含目录 | diff 可控；静态检查口径按"工程文件列表" |
| A7 | 验收从"逐字节回放一致"改为「MSBuild 双配置编译通过 + 静态依赖扫描 + 人工回归路径清单」 | 仓库无录制/回放/测试框架/CI，且 `rng` 用 `random_device` 无法固定种子 | 验证强度下降，已确认接受 |
| A8 | 第一步只建 `Domain`/`Application`/`Adapters` 三个静态库，`Protocol` 留阶段 2 | Q3 排除协议改动，Protocol 本步无内容可放；建空壳属于未经确认的扩展 | 命名约定保留；阶段 2 建立并迁入 DTO/PacketCodec |

另修正原方案两处事实判断（不改变做法，仅记录）：`server_config.json` 的 `//` 注释由 `json::parse(..., ignore_comments=true)` 显式支持，并非解析缺陷；`.trae/specs/points-shop-system` 的 16 个任务与 checklist 已全部完成，商城并非"进行中"（仅 `ClientMain.cpp` 残留两条重复解析的 TODO）。

## 3. 本次执行范围

**已做**：端口声明、遗留决策适配器、`GameLogic`/`Player` 决策 I/O 端口化、`ServerMain` 装配点、三个静态库工程与独立 PCH、解决方案引用。

**明确排除（本次一行未动）**：`client_config.json` / `server_config.json`、`MessageType` 与 `sf::Packet` 序列化、`sendPacketToClient` 的 Partial 重试、`Character::getSkill<>()` 抛异常路径、密码/存档写入、端口 8888 / 14341、`Client2` 工程、Win32 配置、商城 TODO、`.trae` 规格文档。

**留后续**：阶段 2（服务端适配器替换、Protocol 工程、IUserRepository/IConfigProvider/IClock/IRandom）、阶段 3（Domain 净化、IGameContext）、阶段 4（客户端装配）、阶段 5/6（状态机、安全与工程化）。

## 4. 修改范围（文件清单）

**新增（14 个文件）**

- 端口与类型：`header/domain/Types.h`、`header/application/ICommands.h`、`IDecisionPort.h`、`IServerView.h`
- 适配器：`header/adapters/LegacyBlockingDecisionAdapter.h`、`source/adapters/LegacyBlockingDecisionAdapter.cpp`
- 工程与独立 PCH：`Domain/`、`Application/`、`Adapters/` 各含 `.vcxproj` + `pch.*.h` + `pch.*.cpp`
- 交付物：本文件

**修改（7 个文件）**

- `header/GameLogic.h`、`source/GameLogic.cpp`：去掉 `ServerNetwork&`/`getNetwork()`，改持 `IServerView&` + `IDecisionPort&`；`broadcastState()`/`flushCharInfo()` 转发到端口；会话用户名改走 `usernameOf()`
- `header/Player.h`、`source/Player.cpp`：新增 `IDecisionPort& decision`；`ask`/`chooseCard`/`waitForReconnect`/`handleDigitKey`/`handleConfirm` 的 I/O 调用换成端口；循环、分页、超时、600 秒重连、数字键技能、选中索引同步全部原样保留
- `source/ServerMain.cpp`：装配 `LegacyBlockingDecisionAdapter` → 注入 `GameLogic` → `attach()`
- `Server/Server.vcxproj`：新增端口头引用与 `ProjectReference` 到 `UNOOL.Adapters`
- `UNOOL.slnx`：新增三个工程（**保留** Client1/Client2/Server 与 x64/x86 两个平台）

**未改动**：`Client1/Client1.vcxproj`、`Client2/Client2.vcxproj`、`Socket.h/.cpp`、`GameState.h/.cpp`、`UserDB.h/.cpp`、`Character.*`、`Skill.*`、`PassiveSkill.*`、`InstantSkill.*`、`TransformSkill.*`、`Effect.*`、`utils.*`、所有客户端 Scene/Renderer 文件、`header/pch.h`、两个 JSON 配置、图片资源。

## 5. 关键调整

1. **端口只做原子 I/O**：`IDecisionPort` 提供 `poll`/`takeInput`/`showChoice`/`clearChoice`/`isConnected`/`isLoggedIn`/`reconnectTimeoutSec`；`IServerView` 提供 `onState`/`onCharInfo`/`usernameOf`/`spendPoints`。端口不含线上 DTO，Application 不反向依赖 Protocol。
2. **键码零改动**：`DecisionInput.key` 用 `std::uint32_t` 原样透传 `sf::Keyboard::Scancode` 数值，领域侧 `static_cast` 还原；所有 `switch/case`、分页、数字键、技能键判定逐字保留，`ClientInput` 线上包字节不变。
3. **零行为变化的具体保证**：`ChoicePrompt` 字段与 `sendPlayerChoice` 形参一一对应且默认值一致；`broadcastState` 的实现（含末尾 `flushCharInfo`、失败告警日志、每人独立 `packStateForPlayer`）整体平移到适配器 `onState`/`onCharInfo`；重连上限由适配器读同一配置项；`sf::Clock`、16ms/100ms 睡眠、`std::println` 文本全部保留。
4. **PCH 隔离**：Domain/Application 的 PCH 仅标准库；Adapters 的 PCH 含 SFML + nlohmann（内容与 `header/pch.h` 等价但为独立文件、独立 `.pch` 产物）；三个 exe 工程继续使用原有 `header/pch.h`。
5. **Composition Root**：`ServerMain.cpp` 是全项目唯一出现具体适配器装配的位置。

## 6. 潜在风险

1. **编译验证未执行**（最高优先级）：本机 `MSBuild.exe` 两次调用均被环境命令校验拦截（含一次显式授权尝试）。所有代码只经过静态核对，未经编译器与链接器确认。可能残留编译期或链接期问题，需在允许执行构建工具的环境补验。
2. **Domain 净化尚未完成**：`Player.cpp` 仍含 `sf::Keyboard::Scancode`/`sf::Clock`，`GameLogic.cpp`/`Player.cpp` 仍直接读 `getServerConfig()`、调 `UserDB::instance()`，`Character.cpp` 仍扫图片目录。因此原方案"`Domain` 中外部头命中数 = 0"在第一步**不成立**，属阶段 2/3 范围。
3. **静态库跨工程符号解析**：`UNOOL.Adapters.lib` 引用由 exe 侧编译的 `ServerNetwork`/`GameLogic` 符号，链接期由 Server 提供；`Client1/Client2` 不引用该库（只编译端口头，纯虚调用无需链接外部符号）。该假设依赖编译验证确认。
4. **PCH 骨架产物**：Domain/Application 本步只有 PCH 翻译单元，各自带一个 anchor 符号避免空库警告；尚未迁入领域 `.cpp`，语义上是编译边界占位。
5. **`ICommands.h` 为骨架**：按原文 §10 要求写入，但本步无调用方；按 Q5 去掉了阻塞模型不适用的 `tick()`。
6. **Win32 配置保留**：与现状一致仍为 C++20 且已知编译不过；本次未改动、未验证。

## 7. 验证结果

**已通过（静态只读核对）**

- 基线一致：原方案标注的行数与工作区一致；改动前 git 工作区干净（`7259c16`）。
- 工程文件：6 个 `.vcxproj` 与 `.slnx` 的 XML 结构、5 组 GUID、`ProjectReference` 指向、全部 `ClCompile`/`ClInclude` 引用路径均存在；6 个工程各保留 4 个配置（Win32 未删）。
- 端口与实现：`IDecisionPort` 7 个 + `IServerView` 4 个虚函数与适配器 11 个 `override` 一一对应，签名/const 一致。
- 构造签名：3 处 `make_unique<Player>` 全部传入 `decision`；`GameLogic` 构造仅 `ServerMain.cpp` 一处并已改为 `(adapter, adapter)` + `attach()`。
- 耦合清除：`Player.cpp` 中 `network.`/`getNetwork()`/`ServerNetwork`/`getClientSlots()`/`ClientInput` 命中数为 **0**；`GameLogic.cpp` 中同类命中数为 **0**；全仓 `getNetwork()` 仅剩注释文字。
- 广播调用链：`broadcastState()` 的所有调用点（含技能文件数十处）保持不变，`onState()` 末尾仍触发 `onCharInfo()`，无重复发送。
- 未触碰确认：`MessageType`、`sf::Packet` 序列化、端口 8888 / 14341、两个 JSON 配置、`header/pch.h`、Client2 工程、商城 TODO 的 git 状态均为未修改。

**未完成**

- **MSBuild `Debug|x64` / `Release|x64` 双配置编译**：被环境安全策略拦截，未执行。请在本地直接运行：

```
"D:\Admin\Softwares\Compilers\VisualStudio\VS2026\MSBuild\Current\Bin\MSBuild.exe" "D:\Admin\Softwares\Compilers\VisualStudio\Projects\UNOOL\UNOOL.slnx" -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
"D:\Admin\Softwares\Compilers\VisualStudio\VS2026\MSBuild\Current\Bin\MSBuild.exe" "D:\Admin\Softwares\Compilers\VisualStudio\Projects\UNOOL\UNOOL.slnx" -p:Configuration=Release -p:Platform=x64 -m -v:minimal
```

- **运行时回归**：按 Q4 的人工回归清单执行，建议至少覆盖：登录/注册/查重/签到、商城购买、BP 选将/双将/皮肤、出牌与选项分页、超时、断线重连 600 秒、数字键发动技能、+4 质疑、体力结算、游戏结束加分、Client1/Client2 双开。

## 8. 后续阶段

原方案阶段 2–4 的路线与验收保持不变，但在开工前需先补：录制/回放设施（若恢复逐字节验收）、`IUserRepository`/`IConfigProvider`/`IClock`/`IRandom`/`IAssetCatalog` 的注入路径、以及 `GameSnapshot` 与 `GameState` 的映射规则。阶段 5/6 按 Q3 另期排期。
