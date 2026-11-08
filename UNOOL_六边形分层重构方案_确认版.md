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
- `Keys.h` 31 项数值与 `dep/SFML/include/SFML/Window/Keyboard.hpp` 的 `Scan` 枚举**逐项程序化比对一致**；`Player.h/.cpp` 使用的 31 个键码全部有定义。
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
