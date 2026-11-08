# UNOOL 六边形分层重构方案 · 确认版（第一步 + 第二步续作）

> 原方案：`C:\Users\Admin\Desktop\UNOOL_六边形分层重构方案.md`
> 确认日期：2026-11-08。本文件是原方案的**确认修订版与执行记录**。
> 第 1–8 节为第一步（`3e93956`）；第 9 节为第二步续作「补齐已点名残留耦合」。
> 未在下文中列为"调整/排除"的原方案条款保持不变；阶段 5–6 仍未执行，仅作为后续路线。

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

原方案阶段 2–4 的路线与验收保持不变，但在开工前需先补：录制/回放设施（若恢复逐字节验收）、以及 `GameSnapshot` 与 `GameState` 的映射规则。五个出站端口的注入路径已在第 9 节落地。阶段 5/6 按 Q3 另期排期。

---

## 9. 第二步续作：补齐已点名残留耦合

### 9.1 范围确认（本轮两项选择）

| 问题 | 结论 |
|---|---|
| 本轮「继续重构」按哪一级范围执行 | **补齐已点名残留耦合**：沿用第一步端口/适配器约定，处理 `GameLogic.cpp` / `Player.cpp` / `Character.cpp` / `ServerMain.cpp` / `utils` 中方案已点名的 `getServerConfig`、`UserDB::instance()`、`random::rng`、`sf::Clock`、图片目录扫描；并消除 `Player.h/.cpp` 的 `sf::Keyboard::Scancode` |
| 「接口不变」口径 | **仅线上协议与对外行为**：允许按第一步已确认方式新增端口、调整 `GameLogic` / `Player` 内部构造签名；公开方法、`MessageType`、`sf::Packet` 序列化、JSON 配置、端口 `8888` / `14341` 一律不变 |

**本轮明确未动**（与 Q3 / 本轮口径一致）：`Socket.cpp` 业务处理（其 28 处 `UserDB::instance()` 属于 Socket 业务层，不在方案点名范围内）、客户端 Scene、`Client1/Client2` 工程与源码、`Protocol` 工程、任何线上字节、`MessageType` 拆分、长度帧/Partial、子技能异常、安全修复、商城 TODO、删除 `Client2`/Win32。

### 9.2 新增端口与领域类型（7 个头文件，纯标准库）

| 文件 | 内容 |
|---|---|
| `header/application/IConfigProvider.h` | `mode` / `candidateCount(rulesKey)` / `initHandCount(rulesKey)` / `banCount` / `shieldedCharacters` / `shieldedGroups` / `assignedCharacters` / `reconnectTimeoutSec` / `reload`，逐项对应原 `server_config.json` 读取表达式 |
| `header/application/IUserRepository.h` | `getCharacterPool` / `getOwnedCharacters` / `getOwnedSkins` / `getItemCount` / `useItem` / `addPoints` / `addMatchResult`；`Character::Level`、皮肤品质以底层整数透传，端口不认识领域枚举 |
| `header/application/IRandom.h` | `next` / `intIn` / `sizeIn` / `probability` / `seed` |
| `header/application/IClock.h` | `now()` → `std::chrono::steady_clock::time_point` |
| `header/application/IAssetCatalog.h` | `skinsOf(group,name)` / `imagePath(group,name,skin)` |
| `header/application/Services.h` | `DomainServices{config,users,random,clock,assets}` 聚合 + `services()` / `installServices()` 默认服务定位 |
| `header/domain/Keys.h` | `enum class Key : std::uint32_t`，31 项数值与 SFML 3.1 `sf::Keyboard::Scan` 逐字对应 |

### 9.3 关键调整（原因 / 影响）

| # | 调整 | 原因 | 影响 |
|---|---|---|---|
| B1 | `LegacyServicesAdapter` 用 **header-only 内联类** 实现全部 5 个端口；`services()` / `installServices()` 的**非内联定义放在 `source/utils.cpp`** | `Character::getSkins/getImagePath/randomChooseCharacters` 是静态工具，且 Client1/Client2 也编译 `Character.cpp`/`utils.cpp`；若定义在新 `.cpp`，客户端要么改工程、要么新增链接依赖 | 三个 exe 原本都编译 `utils.cpp`，**零新增 .cpp、零客户端工程改动、零新链接依赖** |
| B2 | 保留全局 `unool::random::rng` 名字，类型由 `std::mt19937` 改为无状态 `RandomEngine` 门面（满足 `uniform_random_bit_generator`，`operator()` 转发 `IRandom::next()`） | `Card.cpp` / `GameLogic.cpp` / `Character.cpp` / `InstantSkill.cpp` / `PassiveSkill.cpp` 等 9 处 `std::ranges::shuffle/sample` 直接以 `rng` 作 URBG；`IRandom` 不是 URBG | 这些调用点**一行未改**，仍共用同一条引擎状态，随机序列逐字等价；真正 `mt19937` 移入适配器 |
| B3 | `GameLogic` / `Player` 构造注入 `DomainServices&`；`Character` 静态工具与 `utils` 全局随机门面用 `services()` 默认定位 | 与第一步「兼容门面」同源：静态工具无法构造注入 | `ServerMain` 用 `installServices(domainServices)` 让两类入口指向**同一组实例**；客户端不装配时自动回落到逐字等价的遗留实现 |
| B4 | `LegacyBlockingDecisionAdapter` 增加 `IConfigProvider&` 构造参数，`reconnectTimeoutSec()` 改为端口读取 | 避免适配器直读 `server_config.json`，且避免 Adapters 静态库反向依赖 `utils.obj` | 构造签名 +1 参数，仅 `ServerMain.cpp` 一处调用点 |
| B5 | `sf::Clock` 三处改为 `IClock::now()` + `std::chrono::duration` 差值 | 消除领域层 SFML 时钟依赖 | 秒级用 `duration<double>.count()`、毫秒级用 `duration_cast<milliseconds>`，与原 `asSeconds()` / `asMilliseconds()` 判定一致；16ms/100ms 睡眠与日志文本不变 |
| B6 | `sf::Keyboard::Scancode` → `unool::domain::Key`，`Player.h` 去掉 SFML 头 | 消除领域层 SFML 键码依赖 | `ClientInput.key` 的 `std::uint32_t` 原样透传，31 项键码已程序化比对 SFML 枚举全部一致；`Socket.h/.cpp` 的 `ClientInput` 与序列化**未动** |

### 9.4 修改范围

**新增 8 个文件**：9.2 的 7 个头文件 + `header/adapters/LegacyServicesAdapter.h`。

**修改 14 个已跟踪文件**：

- `header/utils.h` / `source/utils.cpp`：`RandomEngine` 门面、`services()`/`installServices()` 定义、`getServerConfig()`/`reloadServerConfig()` 保留为适配器用的遗留实现
- `header/GameLogic.h` / `source/GameLogic.cpp`：注入 `DomainServices&`，配置与用户数据读取改走 `services.config` / `services.users`，3 处 `make_unique<Player>` 传入 `services`
- `header/Player.h` / `source/Player.cpp`：注入 `DomainServices&`、键码领域化、3 处时钟端口化、`UserDB` 调用改走 `services.users`
- `source/Character.cpp`：图片目录扫描与路径拼装、屏蔽角色/分组读取移到端口
- `source/ServerMain.cpp`：装配 `LegacyServicesAdapter` + `DomainServices`、`installServices`、构造函数、`initCharacters` / `handleGameOver` / `gameLoop` 参数化、`addMatchResult` 整数转换
- `header/adapters/LegacyBlockingDecisionAdapter.h` / `source/adapters/LegacyBlockingDecisionAdapter.cpp`：注入 `IConfigProvider&`
- `Domain/` / `Application/` / `Adapters/` / `Server/` 四个 `.vcxproj`：补 `ClInclude` 条目（仅 IDE 可见性，不改编译单元、不改 PCH、不共享 PCH）

**未改动**：`Client1/Client1.vcxproj`、`Client2/Client2.vcxproj`、`UNOOL.slnx`、`header/pch.h`、三份 `pch.*.h/.cpp`、`Socket.h/.cpp`（含其 28 处 `UserDB::instance()`）、`GameState.*`、`AccountProtocol.*`、`UserDB.*`、`Card.*`、`Skill.*`、`PassiveSkill.*`、`InstantSkill.*`、`TransformSkill.*`、`Effect.*`、所有客户端 Scene/Renderer、两个 JSON 配置、图片资源。

### 9.5 验证结果（静态只读核对，全部通过）

- 6 个 `.vcxproj` XML 可解析，所有 `ClInclude` / `ClCompile` 引用路径存在（0 缺失）。
- 5 个端口共 **24 个纯虚函数**，`LegacyServicesAdapter` **24 个 `override`** 一一对应、签名与 const 一致。
- `Keys.h` 31 项数值与 `dep/SFML/include/SFML/Window/Keyboard.hpp` 的 `Scan` 枚举**逐项程序化比对一致**；`Player.h/.cpp` 使用的 31 个键码全部有定义。**（本条自第 10 节起作废：第三步 C 方案已把 `Keys.h` 改为自增语义枚举，不再复制 SFML 数值，等价性改由映射覆盖率检查保证。）**
- 构造签名：`GameLogic` 仅 `ServerMain.cpp` 一处 `(adapter, adapter, domainServices)`；3 处 `make_unique<Player>` 全部传入 `services`；`installServices` 仅 `ServerMain.cpp` 调用，`services()` / `installServices()` 各只有 1 处定义。
- 耦合收敛：领域层 `getServerConfig()` / `UserDB::instance()` / `sf::Clock` / `Scancode` / `<filesystem>` 命中数为 **0**；遗留直连只剩 `LegacyServicesAdapter.h`（适配器层）、`utils.cpp`（遗留实现本身）与 `Socket.cpp`（本轮明确排除）。
- 协议未触碰：`git diff` 对 `Socket.h/.cpp`、`GameState.h/.cpp`、`AccountProtocol.h/.cpp`、`server_config.json`、`client_config.json` 均为**空**；`MessageType`、端口 `8888`/`14341`、PCH 文件、Client2 工程均无改动。
- 客户端零改动：`Client1/Client2` 两个 `.vcxproj` 与全部客户端源码 git 状态干净。

**未完成（与第一步相同，仍是最高优先级风险）**：`Debug|x64` / `Release|x64` 双配置编译与运行时回归未执行。本机 `MSBuild.exe` 此前两次被环境安全策略拦截；本轮另有 Python 校验命令被同一策略拦截，已改用原生 XML 解析与文本检索完成等价静态核对，**未尝试绕过构建工具的拦截**。命令与人工回归清单见第 7 节。

### 9.6 需要验证的受影响功能与边界情况

| 类别 | 具体项 |
|---|---|
| 配置 | `mode=normal/double`、`candidateCount`（含选将扩充卡加成）、`initHandCount`、`banCount`、`characters.shielded.characters/groups`、`characters.assign` 指定角色（2 个 / 4 个）、每局前 `reload` 后配置生效 |
| 键码 | 数字键 1–9 与小键盘 1–9/0、`0` 取消、`B` 炸弹卡、`Space` 整理手牌、`Up/W` 确认、`Down/S` 取消（非 forced）、`Left/A` 与 `Right/D` 翻页、未知键回落 `Key::Unknown` |
| 时钟 | `ask` 超时（60 秒 ban 询问等 `milliseconds` 路径）、出牌阶段掉线 600 秒重连上限、`waitForReconnect` 重连成功后重发页面与广播 |
| 随机 | 座次 `determineSeatOrder`、候选池洗牌、`randomChooseCharacters` 的 `sample`+`shuffle`、牌堆洗牌、`randomInt/randomSize_t/probability/randomGet` 全链路；客户端与服务端各自只有一条引擎状态 |
| 用户数据 | 角色池 / 已拥有角色 / 已拥有皮肤、增分卡 / 不死图腾 / 选将扩充卡 / 炸弹卡消耗、饮料限定皮肤打万能牌 +1 分、游戏结束 `addMatchResult` 的等级与皮肤品质整数往返 |
| 资产 | `getSkins` 目录不存在 / 无 `.jpg` / 「默认」置首排序、`getImagePath` 与 `getImagePaths` 路径、组合角色双图 |
| 客户端 | Client1/Client2 未装配也能启动：`Character::getSkins/getImagePath/randomChooseCharacters` 与 `RandomEngine` 走默认遗留实现，链接无新依赖 |
| 协议 | `ClientInput` scancode 线上数值、`MessageType`、`GameState`/`CharInfo` 包内容、端口 8888 / 14341 全部不变 |
| 边界 | `probability` 参数越界仍抛 `参数p必须介于0和1之间`；`randomGet` 空容器 / n 超量异常文案；`getImagePath` 未定义角色异常；皮肤目录宽字符 UTF-8/UTF-16 往返（中文角色名与皮肤名） |

---

## 10. 第三步：键码映射内聚到适配器（C 方案，2026-11-08 确认）

### 10.1 触发与确认

第二步把 `Player.h/.cpp` 的 `sf::Keyboard::Scancode` 换成 `header/domain/Keys.h` 时，采用的是「复制 SFML `Scan` 数值」：端口以 `std::uint32_t` 原样透传线上扫描码，领域侧 `static_cast<Key>` 后按数值比较。这消除了**头文件依赖**，但**数值契约仍隐式绑定 SFML** —— 升级 SFML 若调整 `Scan` 编号，`Keys.h` 会静默失配，编译不报错而按键判定出错。

用户就此质询后给出三个方向，确认执行 **C：真正解耦**：

- 领域枚举改为语义自增序号，不再携带任何 SFML 数值；
- `DecisionInput.key` 由 `std::uint32_t` 改为 `domain::Key`；
- `sf::Keyboard::Scancode -> domain::Key` 的映射收进适配器单点函数 `toDomainKey()`；
- 线上字节不变，传输层仍收发 SFML 扫描码数值。

### 10.2 修改范围（4 个文件，无新增/删除文件）

| 文件 | 变更 |
|---|---|
| `header/domain/Keys.h` | 31 项改为纯自增语义枚举；`A` 保持 0，`Unknown` 移到末位作无效哨兵 |
| `header/application/IDecisionPort.h` | 引入 `domain/Keys.h`；`DecisionInput.key` 改为 `domain::Key` |
| `source/adapters/LegacyBlockingDecisionAdapter.cpp` | 匿名命名空间新增 `toDomainKey()`（30 个 case + `default -> Unknown`）；`takeInput()` 改用它；显式包含 `<SFML/Window/Keyboard.hpp>` |
| `source/Player.cpp` | 2 处 `static_cast<Key>(clientInput.key)` 改为直接使用 `clientInput.key` |

**未改动**：`Socket.h/.cpp`（线上包仍 `static_cast<int>(key)` / `static_cast<sf::Keyboard::Scancode>(keyCode)`）、`GameState.*`、`AccountProtocol.*`、`MessageType`、端口 `8888`/`14341`、所有客户端源码与工程、6 个 `.vcxproj`、三份 PCH、两个 JSON 配置、`ICommands.h`、`Player.h`、`LegacyBlockingDecisionAdapter.h`。本轮无新增或删除文件，故工程文件无需调整。

**工作区既有改动（非本轮）**：`source/PassiveSkill.cpp:3013` 的 `(void)carrier.takeCardByIndex(*it);` 在本轮开始前已存在，本轮未触碰；用户已将其与本轮 4 个文件一并提交。

**提交**：本轮代码改动已提交为 `3b6c347 将按键输入转换隔离至适配器层`（第二步成果为 `6dd64ce`）。

### 10.3 等价性证明与关键调整

**旧链路**：`int keyCode` → `static_cast<sf::Keyboard::Scancode>` → `static_cast<uint32_t>` → `DecisionInput.key(uint32)` → `static_cast<Key>`，其中旧 `Key::X` 的数值等于 SFML `Scan::X` 的数值。

**新链路**：收包两步完全不变 → `toDomainKey(Scancode)` → `DecisionInput.key(Key)` → 领域直接使用。

`input->key` 的值就是 `keyCode`；`toDomainKey` 的 30 个 case 恰好是旧 `Key` 定义过的 30 个 SFML 值，且每个 case 同名映射 `Sc::X -> K::X`。因此任一输入要么得到与旧链路相同的 `K::X`，要么落 `default -> Key::Unknown`（旧链路同样不匹配任何判定）。已程序化打印 30 个 case 的 SFML 数值（`A=0, B=1, D=3, S=18, W=22, Num1=26 … Num0=35, Space=40, Right=86 … Up=89, Numpad1=98 … Numpad0=107`），与旧 `Keys.h` 的 30 个显式数值逐项相同。

**唯一内部差异**：`Key::Unknown` 由 `0xFFFFFFFFu` 变为自增末位 30。它仅作无效哨兵，不参与线上字节、不参与任何 case 匹配；`DecisionInput{}` 默认 `key` 仍为 `Key::A`(0)，与重构前 `key=0`（SFML `Scan::A`）一致。无外部行为影响。

**收益与代价**：SFML 版本变化只影响 `toDomainKey()` 一个函数，隐式数值耦合变成显式、可静态校验的映射表；代价是新增一处需与 `Keys.h` 同步维护的 switch，已由 10.4 的覆盖率检查约束。

### 10.4 验证结果（静态只读，全部通过）

- 6 个 `.vcxproj` XML 可解析，条目路径 0 缺失；`LegacyBlockingDecisionAdapter.cpp` 仍由 `Adapters` 静态库编译 1 处，`toDomainKey` 随之进入库；`Server.vcxproj` 通过 `ProjectReference` 链接 `Adapters`，`Client1/Client2.vcxproj` 无任何 `ProjectReference`，不受影响。
- `Keys.h` 枚举 31 项；`toDomainKey` 30 个 case：**未被映射的领域键 0、多余键 0、case 重复 0、返回键重复 0、同名映射不一致 0、SFML 枚举名不存在 0**。
- `Player.h/.cpp` 使用 31 个键码，未定义 0；`static_cast<...Key>` 残留 0。
- 领域/端口头的 `#include` 全为标准库或 `domain/*`、`application/*`，引入 SFML / json / UserDB / Socket / Windows 头 0 处；`Player.h/.cpp` 非注释行 `sf::` 残留 0。
- 协议未触碰：`git diff` 对 `Socket.h/.cpp`、`GameState.h/.cpp`、`AccountProtocol.h/.cpp`、`server_config.json`、`client_config.json` 均为空。

**仍未完成（最高优先级）**：`Debug|x64` / `Release|x64` 编译与运行时回归。本机 `MSBuild.exe` 此前被环境安全策略拦截，本轮未再尝试调用或绕过；命令见第 7 节。

### 10.5 需要验证的受影响功能与边界情况

| 类别 | 具体项 |
|---|---|
| 键码主链 | 数字键 1–9 与小键盘 1–9/0、`0` 取消、`B` 炸弹卡、`Space` 整理手牌、`Up/W` 确认、`Down/S` 取消（非 forced）、`Left/A` 与 `Right/D` 翻页 |
| 未映射键 | `Escape` / `Backspace` / `Tab` / `Enter` / `E` / `H` / `C` 等所有未列出的 SFML 键 → `Key::Unknown`，应表现为「无操作」，与重构前一致 |
| 边界数值 | 线上包中非标准 `keyCode`（负数、越界值）经 `static_cast<Scancode>` 后落 `default`，不应误触发任何判定 |
| 默认构造 | `DecisionInput{}` 的 `key` 仍等价于 `Key::A`；聚合初始化 3 字段全给的调用点行为不变 |
| 协议 | `ClientInput` 包内容、`MessageType`、`GameState` / `CharInfo`、端口 8888 / 14341 不变；Client1/Client2 发送侧仍用 `sf::Keyboard::Scancode` |
| 链接 | `toDomainKey` 位于 `Adapters` 静态库，Server 链接可用；Client1/Client2 既不编译该 `.cpp` 也不链接 `Adapters`，无新链接依赖 |

## 11. 资源定位修复：脱离进程工作目录（2026-11-08，用户确认「全部修」）

### 11.1 触发与范围

配置定位修复后，用户明确选择「全部修」，把同样依赖工作目录的其余资源一并纳入。本轮只修**路径来源**，不改资源内容、协议字节、工程文件与配置内容。

| 残留点 | 旧行为 | 后果 |
|---|---|---|
| `source/UserDB.cpp` 的 `DATA_FILE = "../userDatas.json"` | 相对 CWD | 双击后 `load()` 判为不存在并 `save()` 到 `x64/userDatas.json`，玩家数据看似丢失 |
| `source/ImageManager.cpp` 的 `UNOOL = current_path().parent_path()` | 相对 CWD 的父目录 | 双击后前缀指向 `UNOOL/x64/`，`Card.cpp` / `GameRenderer.cpp` / `ShopScene.cpp` / `Character::getImagePath` 的全部贴图缺失 |
| `header/adapters/LegacyServicesAdapter.h` 的 `skinsOf()` 中 `../images/characters` | 相对 CWD | 皮肤目录扫描失败，签到 / 商城皮肤候选集为空 |

`TextManager.cpp` 的 `config["fonts"]` 是绝对路径 `C:/Windows/Fonts/msyh.ttc`，不受工作目录影响，本轮未改。

### 11.2 关键调整（原因 / 影响）

- **C1**：`unool::paths` 新增 `resourceRoot()`，从 `exeDir()` 起最多上溯 8 级，命中第一个含 `images/` 或 `server_config.json` / `client_config.json` 的目录；未命中回退 `exeDir()`，**绝不回退 `current_path()`**。与既有 `locate()` 同为「近处优先」，部署时把资源放 exe 同目录即可覆盖默认布局。
- **C2**：`ImageManager::UNOOL` 改由 `resourceRoot()` 推导。一处改动覆盖全部图片路径字面量（`Card.cpp` 的 60+ 条、`GameRenderer.cpp` 的标记 / 指针图、`ShopScene.cpp` 的皮肤图、角色图）；这些相对字符串保持原样，因此纹理缓存键与 `Character::getImagePath` 返回值的形态不变。
- **C3**：`skinsOf()` 改为 `resourceRoot()/images/characters/...`，与图片前缀同源；服务端 `Character::getSkins()` 同样受益。
- **C4**：`UserDB` 的 `DATA_FILE` 常量改为 `dataFile()`：**资源根目录优先**，其次 `locate()` 上溯，首次运行落在 `resourceRoot()`。原因是避免误命中旧逻辑可能遗留在 `x64/` 下的空库。`load()` 与 `save()` 共用同一绝对路径；日志中的路径由 `../userDatas.json` 变为绝对路径（仅诊断文本，非对外接口）。

### 11.3 修改范围（5 个文件）

| 文件 | 变更 |
|---|---|
| `header/utils.h` | 新增 `paths::resourceRoot()` 声明 |
| `source/utils.cpp` | 新增 `resourceRoot()` 实现，沿用 `exeDir()` / `error_code` 风格 |
| `source/UserDB.cpp` | `DATA_FILE` → `dataFile()`；`load` / `save` / 3 处日志共 5 处调用点 |
| `source/ImageManager.cpp` | `UNOOL` 赋值来源 1 行 |
| `header/adapters/LegacyServicesAdapter.h` | `skinsOf()` 目录起点 1 行 |

一行未动：`Socket.h/.cpp`、`GameState.*`、`AccountProtocol.*`、`MessageType`、端口 8888 / 14341、两个 JSON 配置内容、6 个 `.vcxproj`、三份 PCH、`Card.cpp` / `GameRenderer.cpp` / `ShopScene.cpp` 的路径字面量、`Character.cpp`、`TextManager.cpp`。因无新增文件，工程文件无需调整。

### 11.4 验证结果（静态只读，全部通过）

- 路径模拟：6 个 exe（Debug/Release × Server/Client1/Client2）从 exe 目录上溯 **2 级**命中资源根；`server_config.json` / `client_config.json` / `userDatas.json` / `images/cards/back.jpg` / `images/characters/元首` 全部命中。
- 等价性：VS「启动」时旧 `current_path().parent_path()` 与 `resourceRoot()` 对 Server / Client1 / Client2 三者完全一致，行为不变；双击时旧前缀指向 `x64/` 且图片缺失、旧 `../userDatas.json` 不存在，与用户现象一致。
- 磁盘核对：`x64/` 各层均无旧逻辑遗留的 `userDatas.json` / 配置副本 / `images` 副本；根目录 `userDatas.json` 4456 字节、4 个账号完好；`dataFile()` 的三种场景（正常 / 有旧空库 / 首次运行）均指向根目录那份。
- 6 个 `.vcxproj` XML 有效、条目路径 0 缺失；`ifstream` / `ofstream` 各 1 处共用 `dataFile()`；可执行代码里 `current_path()` 与 `"../` 残留 0；协议与配置内容 `git diff` 为空。

### 11.5 需验证的受影响功能与边界情况

| 类别 | 具体项 |
|---|---|
| 启动方式 | VS「启动」、双击 `x64/Debug` 与 `x64/Release` 下三个 exe、从任意 CWD 命令行启动，结果一致 |
| 玩家数据 | 注册 / 登录 / 签到 / 商城购买 / 游戏结束加分后写回根目录同一份 `userDatas.json`；不再出现 `x64/userDatas.json` |
| 贴图 | 卡牌正反面、角色图（组合角色多张）、皮肤预览、幽灵标记、指针图、`images/default.jpg` 回退 |
| 皮肤目录 | 中文 group / name 的 UTF-8↔UTF-16 往返；目录缺失 / 无 jpg / 「默认」置首 |
| 部署边界 | 把 json / images 复制到 exe 同目录时优先用同目录那份；资源完全缺失时回退 `exeDir()` 并给出可诊断的失败，不静默写错位置 |
| 配置 | 每局前 `reloadServerConfig()` 仍重读同一份配置；`//` 注释照常支持 |

**仍未完成（最高优先级）**：`Debug|x64` / `Release|x64` 编译与运行时回归。本机 `MSBuild.exe` 被环境安全策略拦截，本轮未再尝试调用或绕过；命令见第 7 节。

## 12. 目录结构调整：六个工程目录归入 Projects/（2026-11-08）

### 12.1 范围与结果

按用户要求，把 6 个工程目录整体移入 `UNOOL/Projects/`：`Server`、`Client1`、`Client2`、`Domain`、`Application`、`Adapters`。git 全部识别为 `R`（rename），16 个被跟踪文件历史保留。其余根目录条目（`header/`、`source/`、`dep/`、`images/`、`x64/`、3 个 json、`.slnx`、README、本文件）**一律未动**。

### 12.2 必须同步修改的引用（否则解决方案无法加载 / 编译）

| 位置 | 改动 |
|---|---|
| `UNOOL.slnx` | 6 条 `<Project Path>` 加 `Projects/` 前缀 |
| `UNOOL.slnLaunch.user` | 3 条启动项目 Path 加 `Projects\\` 前缀 |
| 6 个 `.vcxproj` + 3 个 `.filters` | 共 **284 处**指向根目录的 `..\header\` / `..\source\` / `..\README.md` / `..\.gitignore` / `..\server_config.json` / `..\client_config.json` / `..\userDatas.json` → `..\..\...`（因为工程目录多了一层） |
| 4 处 `ProjectReference` | **保持不变** `..\Adapters\...` / `..\Application\...` / `..\Domain\...`：6 个工程目录仍互为兄弟，`..\` 恰好指向 `Projects/` 内 |

保持不动且已复核：`AdditionalIncludeDirectories` 全部用 `$(SolutionDir)`（指向解决方案根，移动不影响）；`PrecompiledHeaderFile` 是纯文件名，PCH 头随各自工程目录一起移动；工程无显式 `OutDir` / `IntDir`，默认仍输出到 `$(SolutionDir)x64/<Configuration>`。因此 **exe 位置不变**，第 11 节 `resourceRoot()` 上溯 2 级仍命中资源根，运行时行为不受影响。

### 12.3 验证结果（静态只读，全部通过）

- 9 个工程文件 XML 全部可解析，**298 个 ClInclude / ClCompile / None / ProjectReference 条目 0 缺失**；BOM 状态保持（3 个 `.filters` 有 BOM，其余无），编码与换行未改变。
- 精确检查：真正未转换的单层根目录引用 **0**；4 处兄弟工程引用仍为单层 `..\`。
- `.slnx` 6 条、`.slnLaunch.user` 3 条路径全部命中；`dep/`、`header/`、`source/`、`images/`、3 份 PCH 头、根目录 `x64/Debug/*.exe` 全部存在。
- `.gitignore` 的 `x64/` 规则对 `Projects/*/x64/` 仍生效（`git check-ignore -v` 已验证）。

### 12.4 ⚠️ 需用户本地处理：构建中间产物须重新生成

批量改写脚本的文件过滤未生效，遍历到了 `Projects/*/x64/` 下的构建产物，把 `.obj` / `.pch` / `.pdb` / `.idb` 等**二进制当 UTF-8 文本重写，这些中间产物已损坏**。它们被 `.gitignore` 忽略、可由一次编译完整重建，不属于源码或用户数据；被跟踪的源文件经 git 核对无额外改动。

尝试清理时被环境的批量删除安全策略拦截（`SAFE_DELETE_BULK_CONFIRM_REQUIRED`，阈值 50 文件），本轮**未换途径重试、未绕过**。目前除 `Projects/Adapters/x64` 外，其余 5 个 `Projects/*/x64` 与 3 个嵌套同名目录仍在磁盘上（合计约 3.6 GB）。

**请在 VS 里执行「重新生成解决方案」（Rebuild，不是「生成」）强制覆盖全部中间产物，或在资源管理器中手动删除 `Projects/*/x64` 后再生成。** 根目录 `x64/Debug|Release/*.exe` 是最终产物，未被触碰，仍可运行。

### 12.5 需验证项

打开 `UNOOL.slnx` 后 6 个工程均能加载；`Debug|x64` / `Release|x64` 重新生成通过；启动配置仍能同时启动 Server + Client1 + Client2；双击 exe 时配置 / 玩家数据 / 图片定位与第 11 节一致。

**仍未完成（最高优先级）**：双配置编译与运行时回归（本机 `MSBuild.exe` 拦截未解除）。

## 13. 文档归拢 docs/ 与 README 订正（2026-11-08）

### 13.1 范围与结果

按用户「归拢 docs/ 并更新 README」执行，只动文档及其引用，不碰源码、工程配置语义与资源。

| 原位置 | 新位置 | 方式 |
|---|---|---|
| `UNOOL_六边形分层重构方案_确认版.md`（根） | `docs/UNOOL_六边形分层重构方案_确认版.md` | `git mv`，git 记为 `R` |
| `Projects/Server/rules.md` | `docs/rules.md` | `git mv`，git 记为 `R` |
| `Projects/Server/todoList.txt` | `docs/todoList.txt` | 普通移动；该文件被 `.gitignore:372` 忽略、未跟踪。规则不含斜杠，移入 `docs/` 后仍生效（已验证） |

保持原位：`README.md`（仓库首页文档，且被 `Server.vcxproj` / `.filters` 以 `..\..\README.md` 引用，移动只会扩大改动面）、`.trae/specs/**`（工具生成）、`.workbuddy/memory/**`（工作区记忆）、`Projects/*/x64/**`（构建产物）、`.slnx` / `.slnLaunch.user` / `header/` / `source/` / `dep/` / `images/` / 3 个 json。`.gitignore` 未改。

### 13.2 必须同步修改的工程引用

`Projects/Server/Server.vcxproj` 与 `Server.vcxproj.filters` 各 2 处（共 4 处）：`<None Include="todoList.txt">` → `..\..\docs\todoList.txt`；`<Text Include="rules.md" />` → `..\..\docs\rules.md`。二者相对 `Projects/Server/` 需上溯两级到解决方案根。`README.md` 引用保持 `..\..\README.md` 不变；其余 5 个工程未引用文档，零改动。

### 13.3 README 订正（只改过时描述）

| 位置 | 改动 |
|---|---|
| 技术栈 | `3 个 .vcxproj` → `6 个 .vcxproj：3 个可执行工程 + 3 个静态库工程`（已核实 6 份 `ConfigurationType`） |
| 目录结构 | 补 `Projects/`（6 个工程目录）、`UNOOL.slnLaunch.user`、`docs/`、`header/` 下 `domain/` `application/` `adapters/` |
| 核心模块 | 补充分层说明：依赖方向 `Adapters → Application → Domain`、端口清单、`ServerMain.cpp` 为装配根、`GameLogic` 去 `ServerNetwork&`、协议字节未变 |
| 新增「文档」小节 | 索引 `docs/` 三个文件 |
| 构建 | 6 个工程均以 `$(SolutionDir)` 为基准，移入 `Projects/` 后仍开箱即用；DLL 目标明确为 `x64/Debug|Release` |
| 启动 | 说明 8888（服务端硬编码）与 `client_config.json` 的 14341（内网穿透映射）并存是有意配置，本机直连改 `server.port` 即可；「三个项目」改为「三个可执行工程」 |
| 运行前置 | 删除已失效的「工作目录敏感 / `../xxx.json` / `current_path().parent_path()`」整段，改为 `exeDir()` / `locate()` / `resourceRoot()` 的实际定位规则与任意 CWD 一致的行为（对应第 11 节） |
| 配置 / 已知限制 | `server.port` 说明、`userDatas.json` 默认落点（资源根即仓库根）同步订正 |

未改动：玩法、操作、扩展、License、已知限制中除账号落点外的全部条目；README 保持 LF 换行。

### 13.4 验证结果（静态只读，全部通过）

- 6 个 `.vcxproj` + 3 个 `.filters` **XML 全部可解析**；**298 个文件类 Include 条目 0 缺失**（33 个 `.filters` 虚拟筛选器节点已排除，不作为文件路径校验）。
- BOM 状态保持：3 个 `.filters` 有 BOM、6 个 `.vcxproj` 无 BOM；编码与换行未变。
- `docs/` 三份文件均存在，三处原位置均已清空；`git status` 显示 2 个 `R` + 3 个 `M`（README、Server 工程文件），无其他改动。
- README 中已无「工作目录敏感 / `../client_config.json` / `3 个 .vcxproj` / `current_path().parent_path()`」等过时描述；`.gitignore:372` 对 `docs/todoList.txt` 仍生效。

### 13.5 遗留风险

本轮只移动文档与改文档引用，**不涉及任何编译单元、包含目录、链接项或运行时路径**，因此第 11 / 12 节的运行时行为不受影响。第 12.4 节记录的两个遗留项仍未解除：`Projects/*/x64` 中间产物需在 VS 里 Rebuild 或手动删除后重建；`Debug|x64` / `Release|x64` 编译与运行时回归在本机仍因 `MSBuild.exe` 被拦截而未执行。

## 14. 裸指针清零与初始化风格（2026-11-08）

### 14.1 确认结论

用户要求：「代码里不允许出现裸指针，全部改为智能指针（所有权）或引用（观察）或引用包装器（`this`、与第三方库交互以及特殊情况除外，特殊情况需说明）；初始化使用 `{}`」。经只读评估后按 7 项推荐确认执行：

1. 只改项目自有代码（`header/`、`source/`，56 文件 / 17,875 行）的裸指针；
2. `dep/` 第三方源码不动；
3. `&a == &b` 身份比较按特殊情况豁免并登记；
4. 两处可空观察槽用 `opt_ref<T>`（`optional<reference_wrapper<T>>`，项目已有别名）而非改装配顺序；
5. `std::vector<Card*>` → `std::vector<ref<Card>>`；
6. `{}` 只用于本轮新增/修改的代码 + 可改的 `= nullptr` 初始化，不做全库替换；
7. 保持线上协议 / 配置 / 对外行为不变，验收降级为静态验证（`MSBuild.exe` 仍被拦截）。

### 14.2 改动的 5 组裸指针（7 个文件，23 增 / 20 删）

| 组 | 位置 | 改法 |
|---|---|---|
| 1 | `header/adapters/LegacyBlockingDecisionAdapter.h:23,44` + `source/adapters/LegacyBlockingDecisionAdapter.cpp:107,110,117` | `GameLogic* game_ = nullptr` / `game_ = &game` → `opt_ref<GameLogic> game_{}` / `game_ = std::ref(game)`；3 处 `game_->X` → `game_->get().X`。GameLogic 在 adapter 之后构造，只能延迟绑定，用可空引用包装器表达观察语义 |
| 2 | `source/utils.cpp:157,158,169,173` | `DomainServices*& installedSlot()` / `static DomainServices* slot = nullptr` → `opt_ref<DomainServices>&` / `static opt_ref<DomainServices> slot{}`；`services()` 改 `has_value()` + `->get()`；`installServices()` 改 `= std::ref(s)` |
| 3 | `header/InstantSkill.h:564` + `source/InstantSkill.cpp:1194,1231,1233` | `std::vector<Card*> yellowGained` → `std::vector<ref<Card>> yellowGained{}`；`push_back(&...)` → `push_back(std::ref(...))`；`for (Card* cp ...)` → `for (const ref<Card>& cp ...)`，`== cp` → `== &cp.get()` |
| 4 | `source/InstantSkill.cpp:906,911,915,924` | `std::vector<Player*> candidates` → `std::vector<ref<Player>> candidates{}`；`push_back(&p)` → `push_back(std::ref(p))`；`*candidates[i]` → `candidates[i].get()` |
| 5 | `source/PassiveSkill.cpp:1211,1213` | `const Card* ptr = &c.get()` → `const Card& card{c.get()}`；`== ptr` → `== &card` |

另按 Q6 把 `header/Player.h:32` 的 `std::unique_ptr<Character> character = nullptr` 改为 `character{}`（所有权不变，仅初始化风格）。

`ref` / `opt_ref` 由 `header/utils.h:21-25` 提供；`LegacyBlockingDecisionAdapter.h` 新增 `#include "utils.h"`（与同目录 `LegacyServicesAdapter.h` 一致），`InstantSkill.h` 经 `Skill.h` 已可见该别名。`Skill.h` 原本就用 `opt_ref<Player>` 配 `class Player;` 前向声明，因此 `opt_ref<GameLogic>` 对不完整类型可用，无需把 `GameLogic.h` 拉进头文件。

### 14.3 例外与特殊情况登记（按用户要求说明）

- **第三方库交互（豁免）**：SFML 3 的 `sf::Event::getIf<T>()` 返回 `const T*`，局部接住它的 8 处（`ClientMain.cpp:78,93`；`LoginScene.cpp:62,66,70`；`ShopScene.cpp:247,251,255`）；Win32 / nlohmann 需要的 `nullptr` 与缓冲区地址（`utils.cpp:20,81,107,116,118` 的 `GetModuleFileNameW` / `MultiByteToWideChar` / `WideCharToMultiByte` / `&utf8[0]` / `json::parse(..., nullptr, ...)`，`UserDB.cpp:183`）。均在交互边界内，不包装、不改签名。
- **`dep/` 第三方源码**：SFML / mbedtls / libssh2 头文件自身含大量裸指针，不属于本仓库代码，未动。
- **`this` 相关（豁免）**：`*this`（大量）、`GameState.cpp:5` 的 `this != &other`、lambda 捕获 `[this]`。
- **对象身份比较 `&a == &b`（特殊情况，豁免并登记）**：约 8 处（`Player.cpp:644`；`PassiveSkill.cpp:2078,2110,2284,3030`；`InstantSkill.cpp:1173`；`GameRenderer.cpp` 等）。只产生临时指针值用于比较，不是裸指针变量 / 成员 / 参数 / 返回值；彻底消除需要引入对象 ID 或改接口，会动到行为与协议，故按特殊情况保留。本轮第 3、5 组改造后的比较仍是这一惯用法。
- **不是裸指针（无需改）**：`std::optional::operator->`（`event->`、`input->`、`choice->`、`resp->`）、迭代器 `it->second`、`unique_ptr` / `shared_ptr` 的 `->` 与 `*`、`.get()`（引用包装器）、`typeid(*s)`、`*this`、乘法 `*`。
- **`= nullptr` 中未改的 3 处**（扫描共 6 处，实际只有 3 处是初始化）：`header/pch.h:27` 的 `j = nullptr` 是 `std::optional<T>` 序列化无值分支写入 JSON `null`，改成 `j = {}` 会改变序列化字节；`header/Socket.h:176` 与 `source/Socket.cpp:75` 的 `!= nullptr` 是 `unique_ptr` 判空比较，不是初始化。以上均保持原样，属特殊情况。全库其余约 406 处标量 `= 0/false/true`、19 处构造初始化列表 `(...)` 未做全量 `{}` 替换（Q6）：`std::vector<T> v{n}` 与 `v(n)` 语义不同，`{}` 禁止窄化可能直接编译失败，且本机无法编译验证。

### 14.4 验证结果（静态只读，全部通过）

- 正则扫描 `header/` + `source/` 全部 `.h` / `.cpp`：**裸指针声明 0 处**；剩余 `*` 命中全部是解引用（`*unique_ptr`、`*optional`、`*this`）、乘法与 `*it`，已逐条核对。
- 3 处 `game_->` 全部改为 `->get().`；`yellowGained` 6 个使用点、`candidates` 4 个使用点全部同步；`git diff` 仅 7 个源码文件、23 增 20 删。
- 编码：`/utf-8` 在 6 个 `.vcxproj` 中均存在；改动文件全部 UTF-8 无 BOM；`adapters/` 两个文件保持原本的 LF，其余改动文件保持原本的 CRLF（Edit 未整份重写，`git diff` 无全文件换行变化）。
- 包含链：`opt_ref` / `ref` / `std::ref` 均可经 `utils.h`（含 `<functional>`、`<optional>`）到达；`opt_ref<不完整类型>` 在本项目已有先例且可编译。

### 14.5 遗留风险

- **未编译验证**：`MSBuild.exe` 仍被环境安全策略拦截，本轮未再尝试调用或绕过。`Debug|x64` / `Release|x64` 编译与运行时回归仍需用户在 VS 里执行；第 12.4 节的 `Projects/*/x64` 中间产物须 Rebuild 或手动删除后重建。
- **需重点回归的技能**：`炼石` / `成金`（`yellowGained` 改为引用包装器后按地址比较找手牌下标）、`飞手`（`candidates` 改为引用包装器后取目标）、`水鬼`（蓝色牌身份比较改为引用）、以及 Server 首次状态广播 / 角色信息刷新（`game_->get()` 路径）。这些改动均为观察语义替换，线上协议字节与配置内容未变。
- 全库 `{}` 初始化风格统一尚未执行（Q6 范围外），如后续要求需单独评估窄化与容器构造语义差异。

## 15. 工程头文件条目按分层归位（2026-11-08，用户确认 A 方案）

### 15.1 口径澄清（本轮最重要的事实）

`ClInclude` 在 MSBuild 中**不是编译输入**：编译只遍历 `ClCompile`，`ClInclude` 仅用于 IDE「头文件」节点、IntelliSense 导航与静态分析。因此删除任意 `ClInclude` 条目（乃至全部删除）**不影响编译、链接、运行与线上协议**。此前第 14 节及本轮之前的「编译依赖闭包」统计按 `#include` 传递关系计算，与「哪些工程条目可删」是两个不同口径，不可混用。

用户实测删除 `IClock.h` 条目无异常，即属此类。但 `IClock.h` **文件本身仍是真实编译依赖**：`Services.h:3` 包含它、`DomainServices` 持有 `IClock& clock`、`Player.cpp` 6 处 `services.clock.now()`、`LegacyServicesAdapter.h` 继承并实现 `now()`、`ServerMain.cpp:123` 与 `utils.cpp:163` 为 5 项聚合初始化。删除文件或 `#include` 会直接编译失败。

### 15.2 改动范围（4 个工程文件，共删 13 条，全部为 `ClInclude` 行）

| 文件 | 删除条目 | 条数变化 |
|---|---|---|
| `Projects/Server/Server.vcxproj` | `domain\Types.h`、`domain\Keys.h` 及 7 个 `application\*` 端口头 | 27 → 18 |
| `Projects/Server/Server.vcxproj.filters` | 与上完全对应的 9 个 `<ClInclude>` 块（每个含 `<Filter>头文件</Filter>`） | 27 → 18 |
| `Projects/Application/UNOOL.Application.vcxproj` | `application\ICommands.h`、`application\IClock.h` | 12 → 10 |
| `Projects/Adapters/UNOOL.Adapters.vcxproj` | `application\ICommands.h`、`application\IClock.h` | 14 → 12 |

`Domain` 工程 3 条不变。归位依据：`application/` 9 个头与 `domain/` 2 个头在 `Application` / `Domain` 工程中原本就各有条目，删掉 Server 的重复项不会让任何头失去归属；Server 保留 16 个共享业务头 + 2 个 `adapters/` 头（这两者是 `ServerMain.cpp` / `utils.cpp` 直接 `#include` 的组合根依赖）。

### 15.3 结果与需注意的一点

- Server 的 `vcxproj` 与 `filters` 条目集合**完全一致**（各 18 条）；Server 中已无 `application/` / `domain/` 条目。
- `application/IClock.h` 与 `application/ICommands.h` **文件仍在磁盘、未删除**，但按 A 方案清掉 4 条残留后，二者不再出现在任何工程的「头文件」节点。`ICommands.h` 是零引用骨架（第 5 节已说明本阶段无调用方），无节点合理；`IClock.h` 是活跃的 5 端口之一，若希望它在 IDE 里可见，可在 `Application` 工程补回 1 条（纯显示，不影响编译）。
- `Adapters` 工程仍列着 7 个 `application/` 与 2 个 `domain/` 端口头，属同类跨工程重复；本轮按 A 的字面范围未动，可后续按需再清。

### 15.4 验证结果（静态只读）

9 个 `.vcxproj` / `.filters` XML 全部可解析；**285 个文件类 Include 引用 0 缺失**（33 个 `.filters` 虚拟筛选器节点已排除；条目由 298 减至 285，与删除 13 条精确对应）；BOM 状态保持（3 个 filters 有 / 6 个 vcxproj 无）；换行保持（Server 系 CRLF、Application/Adapters/Domain 系 LF），`Server.vcxproj.filters` 末尾仍无换行；`git diff` 只有 4 个工程文件的 `ClInclude` 行变化，**未触碰任何源文件、包含目录、链接项、配置或运行时路径**，因此第 14 节的编译闭包与第 11 节的运行时行为均不变。

### 15.5 遗留风险

本轮为纯 IDE 组织调整，无功能影响。第 12.4 / 14.5 节的两项遗留不变：本机 `MSBuild.exe` 被拦截，`Debug|x64` / `Release|x64` 编译与运行时回归仍未执行；`Projects/*/x64` 中间产物仍需 VS Rebuild 或手动删除后重建。
