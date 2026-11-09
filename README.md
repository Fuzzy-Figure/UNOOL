# UNOOL — UNO Online

**UNOOL**：基于 C++ / SFML 3.1 的双人联机卡牌对战游戏。用标准 UNO 牌堆做载体，自创角色技能与体力规则。

- **70+ 名可选角色**，每名带 1～3 个技能；角色与技能池持续扩充中
- **C/S 架构**，服务端权威；每人只收到自己的真实手牌，对手手牌以背面牌填充
- 账号注册/登录 + 按双方角色等级差查表的积分系统

> 角色、技能的具体数量与完整列表以代码为准<br>
角色表见 `source/domain/Character.cpp` 的 `Character::infos`<br>
技能实现见 `header/domain/` 下的 `PassiveSkill.h`（被动技）/ `InstantSkill.h`（即时技）/ `TransformSkill.h`（转换技）及 `source/domain/` 下对应的 .cpp 文件。


## 技术栈

| 项 | 选型 |
|---|---|
| 语言 | C++23（MSVC `stdc++23`，仅 x64 配置） |
| 格式化 | `std::format` / `std::println` 全面替代字符串拼接；<br>已为 `Card::Color`、`Card::Name`、`Card`、`Cards`、`Hand`、`Character::Level` 提供 `std::formatter` 特化<br>（见 `header/domain/Card.h`、`header/domain/Character.h`），可直接放进 `{}` 占位符 |
| 图形 / 网络 | SFML 3.1.0（Graphics / Network / Window / System / Audio） |
| JSON | nlohmann/json（单头文件） |
| 构建 | Visual Studio 2026（`.slnx` + 7 个 `.vcxproj`：3 个可执行工程 + 4 个静态库工程），**仅 x64** |
| 平台 | Windows 10/11 |

## 目录结构

```
UNOOL.slnx              解决方案：Server / Client1 / Client2 + Domain / Application / Adapters / Core
UNOOL.slnLaunch.user    启动项目配置
Projects/               7 个 VS 工程目录
  Server/               Server.vcxproj（服务端）
  Client1/ Client2/     Client1.vcxproj / Client2.vcxproj（同一份 ClientMain.cpp，仅 ProjectGuid 不同）
  Domain/               UNOOL.Domain.vcxproj（纯标准库领域层，独立 pch）
  Application/          UNOOL.Application.vcxproj（端口与服务定位，独立 pch）
  Adapters/             UNOOL.Adapters.vcxproj（遗留实现适配层，pch 含 SFML / json）
  Core/                 UNOOL.Core.vcxproj（共享编译单元与协议 / DTO 序列化，独立 pch）
header/                 头文件；实体 / 端口 / 适配器 / 协议分别归 domain/ application/ adapters/ protocol/，pch.h 与 utils.h 留在根
source/                 源文件；与 header 同按 domain/ application/ adapters/ protocol/ 四层归置，pch.cpp / utils.cpp / 两端入口留在根
dep/                    外部依赖，包括 SFML 3.1.0（include + lib + bin）和 nlohmann/json
images/                 图片资源：cards / characters / marks
docs/                   设计文档与待办清单（见「文档」一节）
client_config.json      客户端配置（字体、服务端 IP、各元素显示尺寸）
server_config.json      服务端配置（模式、候选数、手牌数、禁用角色）
userDatas.json          运行时生成的账号数据（密码明文，已 gitignore）
```

核心模块：`GameLogic`（回合/座次/牌堆/技能触发）、`Player`（出牌摸牌与交互）、`Card`（牌堆与手牌容器）、`Skill` 系（技能框架）、`GameRenderer` / `ImageManager` / `TextManager`（客户端渲染）、`Socket`（网络封装）、`UserDB`（账号持久化）。

头文件按四层归置后，源码中的 `#include "Card.h"` 等写法保持不变，由各工程的 `$(SolutionDir)header\domain` / `application` / `adapters` / `protocol` 包含目录解析；带前缀的 `#include "application/Services.h"` 仍按 `$(SolutionDir)header` 解析。`pch.h` 与 `utils.h` 属构建基础设施，留在 `header/` 根。四个分层包含目录已在 7 个工程的全部 Debug / Release 配置登记（含 `Server` 的 Win32 两条）；项目实际只构建 x64，Win32 仅保持包含目录一致，链接项与语言标准沿用既有配置。

源文件同样按四层归置，与头文件目录一一对应；`.cpp` 之间不互相 include，故归位 `.cpp` 不需要新增包含目录、也不需要改任何 `#include`，只由各工程 `ClCompile` 的 `..\..\source\<层>\X.cpp` 路径定位。`pch.cpp`、`utils.cpp` 与两端入口 `ClientMain.cpp` / `ServerMain.cpp` 留在 `source/` 根。新增源文件时请放入对应层目录，并同时登记到编译它的工程 `ClCompile`（3 个 exe 还需同步 `.vcxproj.filters`）。

分层（六边形 / Ports and Adapters）：`Domain` 只依赖标准库，定义 `player_id`、`Key` 等领域类型；`Application` 定义端口（`IDecisionPort`、`IServerView`、`IConfigProvider`、`IUserRepository`、`IAccountSession`、`IRandom`、`IClock`、`IAssetCatalog`）与 `DomainServices` 服务定位、用例服务 `AccountService` / `ShopService`；`Adapters` 把 SFML、JSON、文件系统等遗留实现接到端口上。依赖方向恒为 `Adapters → Application → Domain`，装配在 `ServerMain.cpp` 完成。`GameLogic` 不再持有 `ServerNetwork&`，改依赖端口；线上协议字节、配置格式与对外行为均未改变。

`Core` 静态库（原名 `Protocol`，阶段 3 起为共享编译库）统一编译 20 个 `.cpp`：12 个共享业务逻辑（原由 3 个 exe 各编译一遍）、`source/protocol/` 下 `GameState.cpp` / `AccountProtocol.cpp` / `CardPacket.cpp` 三个协议与 DTO 单元、`source/application/AccountService.cpp` / `ShopService.cpp` 两个用例服务单元，以及 `source/application/Services.cpp` / `source/adapters/JsonConfig.cpp` / `LegacyBundle.cpp` 三个归位单元，使用独立的 `pch.core.h`（PCH 文件名与库名一致）。阶段 3 第十二批把默认服务定位器 `services()` 与 JSON 配置缓存门面分别从 `source/utils.cpp` 归位到前两个单元，`utils.cpp` 恢复为只含路径 / 编码 / 随机 / 数学工具、不包含任何 `adapters` 头的领域工具单元；第十三批再把默认组合的实例化（`LegacyServicesAdapter` 及其传递的 `json.hpp` / `UserDB.h`）剥离到 `source/adapters/LegacyBundle.cpp`，`Services.cpp` 因此也变成纯端口依赖，具备划入纯标准库 PCH 的 `Domain` / `Application` 库的条件。集中后 `Core.lib` 不再反向引用 exe 的对象文件符号，3 个 exe 只编译各自的入口 / 渲染 / 场景单元。`MessageType` 枚举已归位到 `header/protocol/MessageType.h`（同一个 25 值枚举逐字平移，值 / 顺序 / 隐式数值 / 线上字节全不变），`AccountProtocol` 因此不再反向包含 `Socket.h`；网络收发仍留在 `Socket.*`，枚举未拆分成 `ClientCommand` / `ServerEvent` 两组。W5 第一段把账号用例（注册 / 登录 / 查重 / 准备 / 积分消费）从 `Socket.cpp` 抽到 `AccountService`，经 `IAccountSession`（由 `ServerNetwork` 实现）与 `IUserRepository` 两个端口完成，组合根在 `ServerMain.cpp` 装配；`Socket.cpp` 只保留会话状态、端口实现与薄转发，线上包内容逐字不变。W5 第二段（分段第一段）把商城展示 `sendShopData`、技能描述 `handleSkillInfoRequest`、每日签到 `handleCheckin` 约 153 行抽到 `ShopService`，与账号服务共用 `IAccountSession` / `IUserRepository` 两个端口；分段第二段把购买决策 `handleShopPurchase` 约 146 行也抽到 `ShopService`（至此 W5 第二段收官），`IUserRepository` 扩到 20 个方法、`IAccountSession` 扩到 12 个方法；`ServerNetwork` 只保留 `sendShopResult` 的纯传输端口实现、会话状态与各薄转发，`sendShopData` 保留为对外接口，`Socket.cpp` 不再含商城业务判断。`ShopData.h` 因含 `<json.hpp>` 与序列化宏，从 `header/domain/` 归位到 `header/protocol/`，`Domain` 头闭包恢复纯标准库；3 个使用点的 `#include "ShopData.h"` 靠分层包含目录解析，写法未改。

头文件自足化（阶段 3 第十四批）：`header/domain/`、`header/application/`、`header/protocol/` 下的头显式包含自身直接使用的标准库类型；`GameState.h` 与 `AccountProtocol.h` 改用 `namespace sf { class Packet; }` 前向声明（与 `Card.h` 一致），后者原先的 `<SFML/Network.hpp>` 下沉到 5 个使用它的 `.cpp`。头闭包不再依赖 PCH 的传递包含，`Domain` 纯标准库 PCH 口径（Q8）与线上字节、函数行为均未改变；这只是 B1 拆库的编译期前置，库归属尚未调整。

第十五批只读评估（阶段 3）：确认第十四批只解决**编译期**依赖——`Core` 的 20 个业务单元构成强连通的符号闭包（11 个纯标准库候选单元 + 9 个必须依赖第三方头的单元），B1 拆库的死锁在**链接期**；在 Q8（`Domain` 纯标准库 PCH）与零函数体改写两条约束下，`Domain` 没有任何 `.cpp` 可以实装。方案文档第 45 节给出 4 静态库 / 3 exe 的精确工程矩阵、三个口径的可点选项与方向 A / B / C。本批不改代码与工程文件，库归属、`ProjectReference`、PCH 与包含目录一律保持现状，待用户确认方向后再动。

**阶段 3 收官（用户选 A）**：`Domain` / `Application` 确认为「设计层目录 + 独立 PCH 占位库」——这是 Q8 与模型层强连通闭包下的**终态**，不是未完成项；四层目录只约束源码归属与依赖方向，所有 `.cpp` 仍由 `UNOOL.Core` 统一编译一次、3 个 exe 共同链接。B1 拆库结案，不再尝试。阶段 3 十四批（库改名 / 头与源码四层归位 / `MessageType` 物理归位 / W5 账号与商城用例 / 服务定位器与配置门面归位 / 默认组合剥离 / 头文件自足化）已逐批经本地 Debug / Release x64 Rebuild 闭环。

阶段 4 客户端装配评估（第十六批只读）：6 个客户端专属单元共 **2428 行**，其中 `GameRenderer` / `TextManager` / `ImageManager`（721 行）已是零网络耦合的纯 View；`ClientMain.cpp`（入口 + 组合根 + 游戏阶段 13 个 `case` 的收包 switch）、`LoginScene.cpp`、`ShopScene.cpp` 各自持有 `ClientNetwork&` 并自建收包循环、各自回心跳，形成 3 处重复分发与「非账号包不消费、留给 `gamePhase`」的隐式队列约定。方案文档第 46 节给出 S1 组合根与端口骨架 → S2 统一收包分发 → S3 两个 Scene 纯 View 化 → S4 输入适配的分段设计，以及首批范围 / 心跳归属 / 死分支与零引用骨架三个待确认口径。本批零代码、零工程文件改动；`MessageType` 25 值、端口 8888 / 14341、JSON 存档与全部既有口径继续有效。

**阶段 4 · S1（第十六批代码，用户选 ①A + ②A + ③B，已编译通过并提交 `cfa91f3`）**：新增 `header/application/IClientSession.h`（客户端会话端口，纯标准库、6 个纯虚方法，与服务端 `IAccountSession` 区分）、`header/adapters/ClientApp.h` + `source/adapters/ClientApp.cpp`（客户端组合根），`ClientNetwork` 只加继承与 `override`、零函数体改写，`source/ClientMain.cpp` 从 264 行瘦身为 28 行的入口组合根。原 `main` 的 `handleChoicePacket` / `gamePhase` 与装配顺序逐字平移到 `ClientApp::run()`；三个 Scene、三处收包循环、三处心跳回复与 `peekMessageType()` 跨阶段留包语义一律未动，②A 的 `PacketDispatcher` 顺延 S2。按用户 ③B 授权删除 `gamePhase` 中两支只打印的商城死分支（`ShopData` / `ShopResult`），这是本批**唯一对外行为变化**：客户端少 4 条控制台输出，商城功能仍由 `ShopScene` 自己的收包循环处理，线上协议与游戏状态不变；`ICommands.h` 零引用骨架保持原样。`Client1` / `Client2` 的 `ClCompile` 同步登记 `ClientApp.cpp` 并保持一致，`Application` / `Adapters` 静态库只补 IDE 显示用的 `ClInclude`。静态验证：7 个工程 158 条 Include 路径 0 缺失、XML 全解析、裸指针 0、`git diff --check` 干净、`MessageType` 25 值与端口未变；助手侧构建工具仍被拦截、未本地编译。详见方案文档第 47 节。

**阶段 4 · S2（第十七批代码，已编译通过并提交 `6f9e71b`）**：用户确认 S1 编译通过后，助手提交 `cfa91f3` 并按 ②A 推进。新增 `header/adapters/PacketDispatcher.h` + `source/adapters/PacketDispatcher.cpp`，把三处收包循环里重复的「peek 过滤 / 心跳回复 / 包体消息号消费」收拢到单点：`next(net)` 为 DrainAll（对应 `gamePhase` 与 `ShopScene::handleNetwork`），`next(net, { 订阅集 })` 为 Filtered（对应 `LoginScene::pollAccountPackets`，**非账号包不消费、留在队列给 `gamePhase`** 的跨阶段留包约定逐字保留）。三处业务判定段零增删改，只换循环头并删各自的心跳分支 / `case`；心跳回复统一到 `replyHeartbeat()` 一处，包体字节不变。采用迭代器式而非订阅回调，是为了让 `continue` / `break` / `case` 顺序全部留在原函数里，把改动面压到最小；订阅回调留到 S3 两个 Scene 纯 View 化时再做。`ClientNetwork::reconnect()` 里还有第四处一次性收包循环（暂存非登录包、不回心跳），属传输层内部流程，本批零改动，心跳仍会因放回队列而由主循环统一回复。（第二十一批 S3b1 按用户 ③C 已删除该不可达函数，此句为 S2 当时状态记录。）`Client1` / `Client2` 同步登记新单元，`Adapters` 库只补 IDE 显示条目。**本批无对外行为变化**；静态验证：10 个工程 XML 全解析、163 条 Include 路径 0 缺失、双工程 `ClCompile` 一致、裸指针 0、`git diff --check` 干净、协议与端口文件未改。助手侧构建工具仍被拦截、未本地编译；用户已确认 Debug / Release x64 Rebuild 通过（新编译单元 PCH / `/FI`、`initializer_list` 重载解析、结构化绑定与 `sf::Packet` 移动、`/we4715` 均通过）。运行时重点仍是登录阶段的跨阶段留包与三处心跳超时。详见方案文档第 48 节。

**阶段 4 · S3 前置评估（第十八批只读，S2 编译通过后继续）**：用户确认 S2 编译通过后提交 `6f9e71b`。本批零代码、零工程文件改动，只做 S3 精确盘点：两个 Scene 实测 **1591 行**（头 170 + 源 1421；46.2 的 1443 是当时两个 `.cpp` 的行数，S2 删心跳 / peek 段后降为 1421），三个纯 View 参照单元 6 个文件网络符号 0 命中。耦合分五类：`ClientNetwork&` 成员与构造、15 处 `net.*` 调用（send 6 / sendShopPurchase 4 / update 2 / isConnected 2 / setPlayerId 1）、协议构造与解析（`LoginScene` 4 个 `AccountProtocol` 请求 + 2 个手写包 + 3 处响应解析；`ShopScene` 3 处 `json::parse` + 3 处 `get<T>()` + 4 处 `dump()` + 1 个手写包）、两个 Scene 主循环里的 `net.update()` / `isConnected()`、6 个 `enum class` 共 50 处枚举分支；另有 `LoginScene.cpp:150` 的 Scene 内嵌 Scene 与 `ShopScene::skillsCache` 进程级静态业务缓存。**三个结构性障碍**：① S1 的 `IClientSession` 是纯标准库 6 方法端口，不覆盖 Scene 需要的 `send(sf::Packet&)` 等方法，直接扩会让 `Application` 层依赖 SFML；② 商城嵌套上移可能把 `ClientApp::run()` 从线性顺序改成状态机，属时序风险；③ `PacketDispatcher::next(ClientNetwork&)` 绑定具体类型、静态缓存归属未定。附带发现 `setCredentials()` 全项目零调用点（既有潜在缺陷，按 46.7 ③B 不擅改、仅记录）。建议把 S3 拆为 **S3a 端口边界与场景切换 → S3b `LoginScene` 纯 View 化 → S3c `ShopScene` 纯 View 化**，每段单独提交与回归；49.8 给出推进节奏 / 端口归属 / 场景切换三个待确认口径，默认 ①A + ②A + ③A（先跑 49.7 已积压 7 批的回归清单、端口放 adapters、商城嵌套等价上移）。详见方案文档第 49 节。

**S3 口径已确认（49.10）**：用户一次点选 **①A + ②A + ③A** —— 先实跑 49.7 已积压 7 批的运行时与线上字节清单建立基线、**S3 暂缓**；S3a 开工时端口放 `adapters` 层网关（`IClientSession` 保持纯标准库不动）、`LoginScene.cpp:150` 的商城嵌套等价上移到 `ClientApp`（`run()` 线性顺序不变）、`ICommands.h` 零引用骨架继续不动。第十九批的前置条件是回归清单实跑完成，开工按 S3a → S3b → S3c 分段。本批零代码、零工程文件改动。

**阶段 4 · S3a（第十九批代码，已编译通过并提交 `d8699a2`）**：按 49.10 既定口径 ②A + ③A 落地，**两个 Scene 内部业务判定零改动**。新增 `header/adapters/IClientGateway.h`（②A：`adapters` 层网关端口，继承 `unool::application::IClientSession` 以复用其纯标准库会话 6 方法，`sf::Packet` 只用前向声明，新增 `send` / `sendShopPurchase` / `setPlayerId` 三个纯虚，正好覆盖两个 Scene 的 15 处 `net.*` 调用）；`Socket.h` 基类改由它继承、3 个方法加 `override`，`IClientSession.h` / `ClientApp.h` / `Socket.cpp` 函数体零改动、`session_{ net }` 端口视图继续有效。新增 `header/adapters/IShopOpener.h`（③A：场景切换意图端口，单个 `openShop(username)`）；`ClientApp.cpp` 在匿名 namespace 定义文件局部 `ShopOpener`，函数体逐字是原商城两行；`LoginScene.cpp:150` 的同步嵌套改为 `shopOpener.openShop(username)`，断线 / 关窗判定原位置保留，`ClientApp::run()` 线性顺序不变、只多一个栈上局部对象。5 个工程文件补 2 个新头的 IDE 显示条目（`ClCompile` 集合不变）。**本批无对外行为变化**：构造参数指向同三个对象、栈上生命周期与返回大厅时序一致。结果回传通道与障碍 3（`PacketDispatcher` 具体类型绑定、`skillsCache` 归属）顺延 S3b / S3c。静态验证：7 个 `.vcxproj` + 3 个 `.filters` 全解析、169 条 Include 路径 0 缺失、双工程条目集合一致、裸指针 0、`git diff --check` 干净、11 个改动文件统一 UTF-8 无 BOM + CRLF（filters 保留 BOM）、协议与端口文件未改。助手侧构建工具仍被拦截、未本地编译；用户已确认 Debug / Release x64 Rebuild 通过（`sf::Packet` 前向声明与完整 SFML 定义共存、继承链变更与虚表 / `LNK2019`、3 个新 `override`、`LoginScene.cpp` 删 `ShopScene.h` 后的传递依赖均通过），第十九批以 `d8699a2` 闭环。运行时重点是大厅商城按钮 → 展示 / 购买 → 退出回大厅，以及商城期间断线 / 关窗的退出判定。详见方案文档第 50 节。

**阶段 4 · S3b 前置评估（第二十批只读，S3a 编译通过后继续）**：本批**零代码、零工程文件改动**，只做 S3b 精确盘点并集中交口径。S3a 后 `LoginScene` 实测 **616 行**（头 70 + 源 546），其中 `drawCheckinPopup()` + `render()` 共 221 行零网络符号；耦合面为 `LoginScene.h` 仍 `#include "Socket.h"` 的传递依赖、`ClientNetwork& net` 成员、**8 处 `net.*`**（send 5 / update 1 / setPlayerId 1 / isConnected 1）+ 1 处 `PacketDispatcher::next(net, { 5 类订阅 })`、**5 个请求构造**（3 个 `AccountProtocol` helper + `ReadyToPlay` / `CheckinRequest` 手写包）、**4 处响应解析**（`parseCheckUsernameResponse` 1 / `parseAccountResponse` 2 / `CheckinResponse` 手写 1）与 `ConnectionInfo` 的 `packet >> pid`，以及 `Mode` / `Status` / `Focus` 三个状态机共 51 处枚举引用。**四个卡点**：① 端口切到 `IClientGateway&` 但该端口不含 `receivePacket()` / `peekMessageType()`；② 49.10 明确「顺延到 S3c 前再判」的障碍 3 —— `PacketDispatcher::next(ClientNetwork&)` 绑定具体类型；③ 49.4 要求的请求 / 解析上移缺「结果回传通道」（S3a 只建了意图通道）；④ `run()` 主循环的 `net.update()` + `pollAccountPackets()` 驱动位置上移会改变每帧时序。**不可动语义**：跨阶段留包、三处心跳与 3 秒超时、5 个请求包字节与 4 个响应字段顺序、全部文案与 S3a 刚固化的商城返回判定。方案 51.5 / 51.6 / 51.7 给出三组口径：① 本批范围（A 推荐类型解耦先行、协议上移拆 S3b2 / B 一步到位纯 View / C 只抽 helper / D 先做 S4）；② 障碍 3 收包通道（A 推荐扩 `IClientGateway` 并泛化 `PacketDispatcher` / B 独立入站端口 / C 组合根驱动 + 结果回调 / D 本批不泛化）；③ `setCredentials()` 零调用点（A 推荐继续原样保留 / B 补一次调用属行为变化 / C 删死方法）。默认 **①A + ②A + ③A**；选 ①A 时 S3b 拆 S3b1 类型解耦 → S3b2 协议上移两段。未确认前不写任何 S3b 代码。详见方案文档第 51 节。

**S3b 口径已确认（51.11）**：用户一次点选 **①A 类型解耦先行 + ②A 扩网关并泛化 `PacketDispatcher` + ③C 删掉死方法**。③C 偏离推荐，助手先做只读核查：`setCredentials()` 与 `reconnect()` 均零调用，`username` / `password` 只被这两个函数与 `getUsername()` 引用；因 `setCredentials()` 从未调用，两字段恒空、`reconnect()` 恒在首行返回 `false` —— **整条链是不可达死代码，删除对外行为零变化**。安全落地：删 `setCredentials()` / `reconnect()` 声明与定义（含 S2 记录的第四处一次性收包循环）、删两个字段；`getUsername()` 作为 `IClientSession` 端口契约保留，实现改为返回静态空串（与现状恒空串逐字一致），`ClientApp.cpp:131` 的打印不动；`serverIp` / `serverPort`（`connect()` 仍赋值，只写不读）、`<thread>`（`ServerNetwork` 仍用）、`AccountProtocol.h`（`ServerNetwork` 仍用）与 `connect()` 函数体全部保留。**风险**：删掉的是客户端唯一的自动重连能力（当前不可达），将来恢复需重写，属用户 ③C 明确授权。①A + ②A 落地为 `IClientGateway` 加两个收包纯虚、`PacketDispatcher` 三个签名泛化、`LoginScene` 成员与构造切端口、其余业务代码逐字不动；协议上移归 S3b2。第二十一批 S3b1 无需再点选、确认后直接开工。

**阶段 4 · S3b1（第二十一批代码，②A + ①A + ③C，已编译通过）**：**7 个代码文件、零工程文件改动**，对外行为零变化。②A：`IClientGateway.h` 加 `std::optional<sf::Packet> receivePacket()` / `std::optional<int> peekMessageType() const` 两个纯虚并 include `<SFML/Network.hpp>`（`std::optional<sf::Packet>` 要求完整类型），`Socket.h` 两方法加 `override`（`ClientNetwork` 共 11 个），`PacketDispatcher` 两个 `next()` 与 `replyHeartbeat()` 签名由 `ClientNetwork&` 泛化为 `IClientGateway&`、函数体逐字未改；三处调用点因派生→基类隐式转换零改动。①A：`LoginScene.h` 删 `#include "Socket.h"`、成员与构造参数切到 `IClientGateway&`，`LoginScene.cpp` **仅构造签名 1 行**变化，5 个请求构造 / 4 个响应解析 / 状态机 / `render()` / `run()` 主循环逐字留在原处，协议上移归 S3b2。③C：删零调用的 `setCredentials()` / `reconnect()` 与恒空的 `username` / `password`（含 S2 记录的第四处一次性收包循环，共 61 行），`getUsername()` 作为端口契约保留、改为返回静态空串（与现状恒空串逐字一致），`ClientApp.cpp:131` 打印不动；`serverIp` / `serverPort` 只写不读顺延。静态验证：10 个工程 XML 全解析、169 条 Include 路径 0 缺失、双客户端条目集合一致、裸指针 0、`git diff --check` 干净、7 个改动文件统一 UTF-8 无 BOM + CRLF、协议与端口文件未改。助手侧构建工具被客户端 LOLBin 硬黑名单拦截（用户两次加白名单均无效、助手未绕过），改由用户本地 `MSBuild.exe /t:Rebuild`：**`Debug|x64` 16.97s、`Release|x64` 23.22s，均「已成功生成 / 0 个警告 / 0 个错误」**，7 个工程（4 静态库 + 3 exe）全部生成，日志见 `%TEMP%\unool_debug.log` / `unool_release.log`。原 Rebuild 重点（`std::optional<sf::Packet>` 纯虚返回与完整 SFML 定义共存、11 个 `override` 与虚表 / `LNK2019`、两处 `next(ClientNetwork&)` 的重载解析与隐式转换、`LoginScene.h` 删 `Socket.h` 后的传递依赖）已全部排除。运行时重点仍是跨阶段留包、三处心跳与 3 秒超时、5 个请求包字节与 4 个响应字段顺序。详见方案文档第 52 节。

## 文档

| 文件 | 内容 |
|---|---|
| `docs/rules.md` | 场 / 局 / 轮 / 回合 / 阶段的完整规则、牌堆构成与万能牌结算 |
| `docs/todoList.txt` | 待做项清单（已 gitignore，仅本地留存） |
| `docs/UNOOL_六边形分层重构方案_确认版.md` | 六边形分层重构的确认决策、调整原因与影响、逐轮修改范围、验证结果与遗留风险 |

## 构建与运行

### 构建

1. 用 Visual Studio 2026 打开 `UNOOL.slnx`
2. **选 x64 平台**（Debug|x64 或 Release|x64）—— 7 个工程均以 `$(SolutionDir)`（解决方案根）为基准使用 `dep\SFML\include`、`dep\SFML\lib`、`dep\nlohmann`、`header` 相对路径，工程虽位于 `Projects/` 下仍开箱即用
3. 生成解决方案（Ctrl+Shift+B）

> **不要选 Win32**：该平台配置的 `LanguageStandard` 为 `stdc++20`，而代码使用了 C\++23 的 `std::views::enumerate`（`GameLogic.cpp`、`GameRenderer.cpp`、`PassiveSkill.cpp` 等），在 C\++20 下会编译失败。

### 运行前必做：补 DLL

在 `dep\SFML\bin` 里有 SFML3 的 .dll 文件，把它们放入与 .exe 同级目录下（默认 `x64/Debug/` 或 `x64/Release/`）。

### 启动

三个可执行工程：`Server`（服务端）、`Client1` / `Client2`（同一份 `ClientMain.cpp` 的两份拷贝，仅 ProjectGuid 不同，用于本地双开测试）；`Domain` / `Application` / `Adapters` / `Core` 为静态库工程，由 exe 引用。

1. 先启动 `Server`，监听 **8888** 端口（硬编码于 `ServerMain.cpp`，非配置）
2. 再启动两个 Client，在 `client_config.json` 中填好服务端 IP / 端口
3. 两端都登录后自动进入 Ban/Pick 与游戏循环

> 仓库里的 `client_config.json` 指向作者的公网地址与 **14341** 端口，与服务端监听的 8888 不同，这是通过内网穿透做端口映射的有意配置；本机直连时把 `server.port` 改成 8888 即可。

> **路径定位不再依赖工作目录**：`exeDir()` 取 exe 自身所在目录；`locate()` 从该目录最多上溯 8 级查找配置 json；`resourceRoot()` 上溯查找含 `images/` 或配置 json 的目录作为资源根（找不到时只落在 exe 目录以便诊断，绝不回退 `current_path()`）。因此 VS「启动」、双击 exe、从任意 CWD 命令行启动的结果一致；把 json / `images/` 复制到 exe 同目录时会优先使用同目录那份。`userDatas.json` 的读写共用资源根下的同一绝对路径，不会再在 `x64/` 下另建空库。

## 配置

`client_config.json`

| 字段 | 说明 |
|---|---|
| `fonts` | 字体文件路径 |
| `server.ip` / `server.port` | 客户端连接的服务端地址；服务端监听端口恒为 8888，两者经内网穿透映射时可不同 |
| `size.window` / `card` / `pointer` / `character` | 窗口、卡牌、选择指针、角色立绘的显示尺寸 |

`server_config.json`

| 字段 | 说明 |
|---|---|
| `mode` | `normal`（每家 1 个角色）/ `double`（双将，每家 2 个角色合成） |
| `characters.assign` | 可选。指定角色则跳过随机候选与 Ban/Pick 阶段 |
| `characters.shielded.characters` / `characters.shielded.groups` | 屏蔽不参与随机抽选的角色名 / 分组 |
| `rules.normal.candidateCount` / `rules.double.candidateCount` | 各自的候选角色数 |
| `rules.normal.initHandCount` / `rules.double.initHandCount` | 各自的初始手牌数 |
| `rules.banCount` | Ban/Pick 阶段每人禁用对方的次数 |

## 玩法

- **牌堆**：四色 0～9 + 反转 / 封禁 / +2 + 变色 / +4，标准 UNO 构成
- **出牌**：无有效颜色（首张）／同色／同名／万能牌
- **体力**：每局结束时手牌分值之和转为伤害扣血，体力归零即败北
- **+4 可质疑**：出牌者手牌含原色则质疑成功（出牌者摸 4），否则质疑失败（目标摸 6 并被封禁）
- **技能**：被动技按触发时机（`PassiveSkill::TriggerTime`，41 种）自动发动，锁定技无需确认，限定技每局一次；主动技在出牌阶段按数字键发动或切换激活态
- **标记**：角色身上的持久化计数（如「速度」），由技能读写，会在角色信息面板显示；部分标记每局重置
- **封印**：被封印时该角色的所有技能（被动锁定技、主动技）当回合失效，剩余回合数显示在角色信息面板
- **Ban/Pick**：拼点决定座次 → 双方互 Ban → 一号位先选角（可选皮肤）

### 操作

| 按键 | 功能 |
|---|---|
| `←` `→` / `A` `D` | 左右选牌（客户端本地生效） |
| `Space` | 按颜色+牌名排序手牌 |
| `↑` / `W` | 确认出牌 / 确认选项 |
| `↓` / `S` | 取消（仅非强制选项） |
| `0`~`9` / 小键盘 | 选项编号 / 发动主动技 / 分页时翻页 |
| 鼠标点击立绘 | 查看角色信息（等级/HP/标记/封印/技能） |

## 扩展

**加角色**：在 `Character.cpp` 的 `Character::infos` 加一行 `{"角色名", {"分组", Level::X, 被动<被动技能名1, 被动技能名2, ...>, 即时<即时技能名1, 即时技能名2, ...>, 转换<转换技能名1, 转换技能名2, ...>, HP}}`，并在 `images/characters/分组/角色名/` 放置 `默认.jpg`（可放多张 `.jpg` 作皮肤，「默认」会自动排在首位）。**分组名必须与目录一致**，否则运行时抛 `角色 <X> 的皮肤目录不存在`。

**加技能**：继承 `PassiveSkillImpl<X>` / `InstantSkillImpl<X>` / `TransformSkillImpl<X>`，**并标记为 `final`**（非 final 会编译失败），实现 `filter()` 与 `content()`，在角色 info 中挂 `X::make`。

**子技能**：任意技能可携带任意类型的子技能（构造时追加），父技能触发时会递归发动子技能。典型用法是让主技能负责「布置状态」，子技能负责在其他时机「消费状态」——例如 `地雷`（`phase_end` 选色布置）配 `地雷_引爆`（`use_card_begin`，他人打出该色时结算）与 `地雷_撤雷`（`phase_begin` 清空）；`冲撞` 本体空实现，逻辑全在 `冲撞_加速` / `冲撞_伤害` / `冲撞_清标记` 三个子技能里。子技能与主技能同级独立匹配触发条件，可用 `carrier.getSkill<父技能>()` 读写父技能状态。

**日志与格式化**：新代码统一用 `std::format` 拼串，避免 `+` 拼接；`Card::Color`、`Card::Name`、`Character::Level` 已特化 `std::formatter`，可直接 `std::format("{}", card.getColor())`。

## 已知限制

- **只能选 x64**：Win32 配置为 C++20，编译不过
- **服务端全程单线程且阻塞**：没有任何线程 / 锁 / 原子量，`ask()` / `chooseCard()` 会同步阻塞等待某个客户端输入。同时只能服务一桌；客户端断线后重连等待默认 600 秒，期间整个服务端无法推进
- **账号安全**：密码明文存 `userDatas.json`、明文 `!=` 比较，并在网络上明文传输，无哈希 / 无盐 / 无 TLS；存档为直接覆盖写，写入中断会损坏全库（该文件已在 `.gitignore` 中，默认写在资源根即仓库根，不随 exe 目录走）
- `client_config.json` 中硬编码了作者的公网服务器 IP
- 非阻塞 socket 上 `send` 返回 `Partial` 时重发整个 Packet，可能破坏协议流
- **子技能强耦合父技能**：子技能通过 `carrier.getSkill<父技能>()` 访问父技能状态，若父技能被任何「失去技能」效果移除，子技能仍会触发并抛 `std::runtime_error("没有找到技能")`
- 无单元测试、无 CI
- 仅支持 Windows

## License

课程/个人学习项目，仅供交流学习。角色名称与图片资源版权归各自原作者所有，卡牌规则灵感来源于经典 UNO。
