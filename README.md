# UNOOL — UNO Online

**UNOOL**：基于 C++ / SFML 3.1 的双人联机卡牌对战游戏。用标准 UNO 牌堆做载体，自创角色技能与体力规则。

- **70+ 名可选角色**，每名带 1～3 个技能；角色与技能池持续扩充中
- **C/S 架构**，服务端权威；每人只收到自己的真实手牌，对手手牌以背面牌填充
- 账号注册/登录 + 按双方角色等级差查表的积分系统

> 角色、技能的具体数量与完整列表以代码为准<br>
角色表见 `source/Character.cpp` 的 `Character::infos`<br>
技能实现见 `header/PassiveSkill.h`（被动技）/ `InstantSkill.h`（即时技）/ `TransformSkill.h`（转换技）及对应的.cpp文件。


## 技术栈

| 项 | 选型 |
|---|---|
| 语言 | C++23（MSVC `stdc++23`，仅 x64 配置） |
| 格式化 | `std::format` / `std::println` 全面替代字符串拼接；<br>已为 `Card::Color`、`Card::Name`、`Card`、`Cards`、`Hand`、`Character::Level` 提供 `std::formatter` 特化<br>（见 `header/Card.h`、`header/Character.h`），可直接放进 `{}` 占位符 |
| 图形 / 网络 | SFML 3.1.0（Graphics / Network / Window / System / Audio） |
| JSON | nlohmann/json（单头文件） |
| 构建 | Visual Studio 2026（`.slnx` + 3 个 `.vcxproj`），**仅 x64** |
| 平台 | Windows 10/11 |

## 目录结构

```
UNOOL.slnx              解决方案：Server / Client1 / Client2
header/                 头文件
source/                 源文件（ClientMain.cpp / ServerMain.cpp 为两端入口）
dep/                    外部依赖，包括 SFML 3.1.0（include + lib + bin）和 nlohmann/json
images/                 图片资源：cards / characters / marks
client_config.json      客户端配置（字体、服务端 IP、各元素显示尺寸）
server_config.json      服务端配置（模式、候选数、手牌数、禁用角色）
userDatas.json          运行时生成的账号数据（密码明文，已 gitignore）
```

核心模块：`GameLogic`（回合/座次/牌堆/技能触发）、`Player`（出牌摸牌与交互）、`Card`（牌堆与手牌容器）、`Skill` 系（技能框架）、`GameRenderer` / `ImageManager` / `TextManager`（客户端渲染）、`Socket`（网络封装）、`UserDB`（账号持久化）。

## 构建与运行

### 构建

1. 用 Visual Studio 2026 打开 `UNOOL.slnx`
2. **选 x64 平台**（Debug|x64 或 Release|x64）—— 三个项目均已使用 `$(SolutionDir)dep\SFML\include`、`$(SolutionDir)dep\SFML\lib`、`$(SolutionDir)dep\nlohmann`、`$(SolutionDir)header` 相对路径，开箱即用
3. 生成解决方案（Ctrl+Shift+B）

> **不要选 Win32**：该平台配置的 `LanguageStandard` 为 `stdc++20`，而代码使用了 C\++23 的 `std::views::enumerate`（`GameLogic.cpp`、`GameRenderer.cpp`、`PassiveSkill.cpp` 等），在 C\++20 下会编译失败。

### 运行前必做：补 DLL

在 `UNOOL\dep\SFML\bin` 里有 SFML3 的 .dll 文件，把它们放入与 .exe 同级目录下

### 启动

三个项目：`Server`（服务端）、`Client1` / `Client2`（同一份 `ClientMain.cpp` 的两份拷贝，仅 ProjectGuid 不同，用于本地双开测试）。

1. 先启动 `Server`，监听 **8888** 端口（硬编码于 `ServerMain.cpp`，非配置）
2. 再启动两个 Client，在 `client_config.json` 中填好服务端 IP
3. 两端都登录后自动进入 Ban/Pick 与游戏循环

> **工作目录敏感**：配置读取 `../client_config.json`、`../server_config.json`，账号数据写 `../userDatas.json`，图片资源根为 `current_path().parent_path()`。请在 `x64/Debug/` 或 `x64/Release/` 下启动，否则直接抛异常。

## 配置

`client_config.json`

| 字段 | 说明 |
|---|---|
| `fonts` | 字体文件路径 |
| `server.ip` / `server.port` | 客户端连接的服务端地址（服务端端口恒为 8888） |
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
- **账号安全**：密码明文存 `userDatas.json`、明文 `!=` 比较，并在网络上明文传输，无哈希 / 无盐 / 无 TLS；存档为直接覆盖写，写入中断会损坏全库（该文件已在 `.gitignore` 中，但仍会随程序目录留存）
- `client_config.json` 中硬编码了作者的公网服务器 IP
- 非阻塞 socket 上 `send` 返回 `Partial` 时重发整个 Packet，可能破坏协议流
- **子技能强耦合父技能**：子技能通过 `carrier.getSkill<父技能>()` 访问父技能状态，若父技能被任何「失去技能」效果移除，子技能仍会触发并抛 `std::runtime_error("没有找到技能")`
- 无单元测试、无 CI
- 仅支持 Windows

## License

课程/个人学习项目，仅供交流学习。角色名称与图片资源版权归各自原作者所有，卡牌规则灵感来源于经典 UNO。
