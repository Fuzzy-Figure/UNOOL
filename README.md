# UNOOL — UNO Online

**UNOOL** = **UNO** + **OnLine**：基于 C++ / SFML 3.1 的双人联机卡牌对战游戏。用标准 UNO 牌堆做载体，自创角色技能与体力规则（更接近三国杀的玩法框架）。

- **数十名可选角色**，每名带 1～3 个技能；角色与技能池**持续扩充中**
- **C/S 架构**，服务端权威；每人只收到自己的真实手牌，对手手牌以背面牌填充
- 账号注册/登录 + 按双方角色等级差查表的积分系统

> 角色、技能的具体数量与完整列表以代码为准：角色表见 `source/Character.cpp` 的 `Character::infos`，技能实现见 `header/PassiveSkill.h`（被动）/ `InstantSkill.h`（即时）/ `TransformSkill.h`（转换）。

## 技术栈

| 项 | 选型 |
|---|---|
| 语言 | C++23（MSVC `stdcpp23`，仅 x64 配置；Win32 配置为 `stdcpp20`，无法编译本代码） |
| 图形 / 网络 | SFML 3.1.0（Graphics / Network / Window / System / Audio） |
| JSON | nlohmann/json（单头文件） |
| 构建 | Visual Studio 2026（`.slnx` + 3 个 `.vcxproj`），**仅 x64** |
| 平台 | Windows 10/11 |

## 目录结构

```
UNOOL.slnx              解决方案：Server / Client1 / Client2
header/                 头文件
source/                 源文件（ClientMain.cpp / ServerMain.cpp 为两端入口）
dep/                    已 vendored：SFML 3.1.0（include + lib）、nlohmann/json
images/                 美术资源：cards / characters / marks
client_config.json      客户端配置（字体、服务端 IP、各元素显示尺寸）
server_config.json      服务端配置（模式、候选数、手牌数、禁用角色）
userDatas.json          运行时生成的账号数据（密码明文）
```

核心模块：`GameLogic`（回合/座次/牌堆/技能触发）、`Player`（出牌摸牌与交互）、`Card`（牌堆与手牌容器）、`Skill` 系（技能框架）、`GameRenderer` / `ImageManager` / `TextManager`（客户端渲染）、`Socket`（网络封装）、`UserDB`（账号持久化）。

## 构建与运行

### 构建

1. 用 Visual Studio 2026 打开 `UNOOL.slnx`
2. **选 x64 平台**（Debug|x64 或 Release|x64）—— 三个项目均已使用 `$(SolutionDir)dep\SFML\include`、`$(SolutionDir)dep\SFML\lib`、`$(SolutionDir)dep\nlohmann`、`$(SolutionDir)header` 相对路径，开箱即用
3. 生成解决方案（Ctrl+Shift+B）

> **不要选 Win32**：该平台配置的 `LanguageStandard` 为 `stdcpp20`，而代码使用了 C++23 的 `std::views::enumerate`（`GameLogic.cpp`、`GameRenderer.cpp`、`PassiveSkill.cpp` 等），在 C++20 下会编译失败。

### 运行前必做：补 DLL

`dep/SFML` 只带了 `include` 和 `lib`，**没有 `bin` 目录**，项目也没有 PostBuildEvent。链接能过，但**运行时会报"找不到 sfml-graphics-3.dl"**。二选一：

- 从 [SFML 3.1.0 官方包](https://www.sfml-dev.org/download.php) 取 `bin/*.dll` 拷到输出目录（`x64/Debug/`）；或
- 改链接 `sfml-*-s-d.lib`（静态版，`dep/SFML/lib` 里已带），并为项目添加预处理器定义 `SFML_STATIC`

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
| `singleCandidateCount` / `doubleCandidateCount` | 各自的候选角色数 |
| `singleInitHandCount` / `doubleInitHandCount` | 各自的初始手牌数 |
| `banCount` | Ban/Pick 阶段每人禁用对方的次数 |
| `characters` | 可选。指定则跳过随机候选与 Ban/Pick（normal 给 2 个、double 给 4 个） |
| `shielded.characters` / `shielded.groups` | 屏蔽不参与随机抽选的角色名 / 分组 |

## 玩法

- **牌堆**：四色 0～9 + 反转 / 封禁 / +2 + 变色 / +4，标准 UNO 构成
- **出牌**：无有效颜色（首张）／同色／同名／万能牌
- **体力**：每局结束时手牌分值之和转为伤害扣血，体力归零即败北
- **+4 可质疑**：出牌者手牌含原色则质疑成功（出牌者摸 4），否则质疑失败（目标摸 6 并被封禁）
- **技能**：被动技按触发时机（30+ 种）自动发动，锁定技无需确认，限定技每局一次；主动技在出牌阶段按数字键发动或切换激活态
- **Ban/Pick**：拼点决定座次 → 双方互 Ban → 一号位先选角（可选皮肤）

### 操作

| 按键 | 功能 |
|---|---|
| `←` `→` / `A` `D` | 左右选牌（客户端本地生效） |
| `Space` | 按颜色+牌名排序手牌 |
| `↑` / `W` | 确认出牌 / 确认选项 |
| `↓` / `S` | 取消（仅非强制选项） |
| `0`~`9` / 小键盘 | 选项编号 / 发动主动技 / 分页时翻页 |
| 鼠标点击立绘 | 查看角色信息（等级/HP/标记/技能） |

## 扩展

**加角色**：在 `Character.cpp` 的 `Character::infos` 加一行 `{"角色名", {"分组", Level::X, {被动技工厂...}, {主动技工厂...}, HP}}`，并在 `images/characters/分组/角色名/` 放置 `默认.jpg`（可放多张 `.jpg` 作皮肤，「默认」会自动排在首位）。**分组名必须与目录一致**，否则运行时抛 `角色 <X> 的皮肤目录不存在`。

**加技能**：继承 `PassiveSkillImpl<X>` / `InstantSkillImpl<X>` / `TransformSkillImpl<X>`，**并标记为 `final`**（三个 `Specific*Skill` concept 已加 `std::is_final_v<T>` 约束，非 final 会编译失败），实现 `filter()` 与 `content()`，在角色 info 中挂 `X::make`。

## 已知限制

- **缺 DLL**：`dep/SFML` 无 `bin`，需自行补（见"运行前必做"）
- **只能选 x64**：Win32 配置为 C++20，编译不过
- 密码明文存储于 `userDatas.json`，且该文件含历史账号数据
- `client_config.json` 中硬编码了作者的公网服务器 IP
- 客户端异常断线会导致服务端永久阻塞（`ask()` 无全局超时，仅 Ban 阶段有 60s）
- 非阻塞 socket 上 `send` 返回 `Partial` 时重发整个 Packet，可能破坏协议流
- 无单元测试、无 CI
- 仅支持 Windows

## License

课程/个人学习项目，仅供交流学习。角色名称与图片资源版权归各自原作者所有，卡牌规则灵感来源于经典 UNO。
