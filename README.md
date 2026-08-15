# UNOOL — UNO Online

> **UNOOL** = **UNO** + **OnLine** —— 一款基于 C++ / SFML 的联机卡牌对战游戏，融合了经典 UNO 规则与自创角色技能系统。

---

## 🎮 项目简介

UNOOL 是一款双人联机卡牌对战游戏。在标准 UNO 牌堆与出牌规则之上，本项目引入了**角色技能系统**：每位玩家在开局前从候选角色池中选取一名角色（含被动技能与主动技能），通过技能与卡牌的配合展开策略博弈。游戏采用**客户端/服务器（C/S）架构**，基于 TCP Socket 实现双端通信，支持账号注册/登录与积分排行。

### 核心玩法

- **标准 UNO 牌堆**：红/蓝/绿/黄四色数字牌（0\~9）、功能牌（封禁 Skip / 反转 Reverse / +2 Draw2）、万能牌（变色 Wild / +4 Wild Draw4）。
- **角色技能系统**：20+ 名角色，每名角色拥有 1\~2 个被动/主动技能，技能可在特定触发时机自动或手动发动，改变出牌、摸牌、体力等规则。
- **体力机制**：每名角色拥有独立体力值（HP），回合结束时手牌分值之和转化为伤害扣除体力；体力归零即败北。
- **Ban / Pick 选角**：开局随机生成候选角色，双方轮流禁用与选择角色，并可为角色选择皮肤。
- **积分系统**：基于角色等级（S/A/B/C/D/F）的积分表，胜方按双方等级差获得对应积分。

---

## 🏗️ 技术架构

| 层级 | 技术选型 |
|---|---|
| 语言 | C++23（`/std:c++latest`） |
| 图形渲染 | SFML 3.x（`sf::RenderWindow` / `sf::Texture` / `sf::Sprite` / `sf::Font` / `sf::Text`） |
| 网络通信 | Windows Socket（`ws2_32.lib`）+ SFML Network（`sf::TcpListener` / `sf::TcpSocket` / `sf::Packet`） |
| 数据序列化 | SFML `sf::Packet` 自定义 `operator<<` / `operator>>` |
| 配置管理 | `nlohmann/json`（`config.json`，单头文件 `json.hpp`） |
| 构建系统 | Visual Studio 2026（MSVC，`.sln` / `.vcxproj`） |
| 平台 | Windows 10/11（控制台代码页强制 UTF-8） |

### 模块划分

```
UNOOL/
├── UNOOL.slnx                  # Visual Studio 2026 解决方案
├── UNOOL.vcxproj               # Visual Studio 项目文件
├── config.json                 # 运行时配置（窗口尺寸 / 牌尺寸 / 服务器 IP & 端口 / 候选角色数 / 初始手牌数）
├── README.md
│
├── source/                     # 源代码 (.cpp)
│   ├── ClientMain.cpp          # 客户端入口：账号阶段 → 游戏主循环
│   ├── ServerMain.cpp          # 服务端入口：监听 → 初始化 → 回合循环
│   ├── GameLogic.cpp           # 核心游戏规则（回合/座次/牌堆/技能触发）
│   ├── Player.cpp              # 玩家逻辑（出牌/摸牌/交互/回合流程）
│   ├── Character.cpp           # 角色数据与技能装配
│   ├── Card.cpp                # 卡牌定义、牌堆/手牌容器、序列化
│   ├── Skill.cpp               # 技能基类 + 全部被动/主动技能实现
│   ├── Effect.cpp              # 卡牌效果函数（封禁/反转/+2/变色/+4）
│   ├── GameState.cpp           # 游戏状态序列化（PlayerState / GameState）
│   ├── GameRenderer.cpp        # 客户端渲染器（角色/手牌/弃牌堆/选项框/信息框）
│   ├── TextManager.cpp         # 文本渲染与自动折行
│   ├── ImageManager.cpp        # 图片纹理缓存与绘制
│   ├── Socket.cpp              # ServerNetwork / ClientNetwork 网络封装
│   ├── AccountProtocol.cpp     # 账号协议包构造与解析
│   ├── UserDB.cpp              # 用户数据库（JSON 文件持久化）
│   └── utils.cpp               # 工具函数（配置/字符串/随机/数学/输入）
│
├── header/                     # 头文件 (.h)
│   ├── Player.h
│   ├── GameLogic.h
│   ├── Character.h
│   ├── Card.h
│   ├── Skill.h
│   ├── Effect.h
│   ├── GameState.h
│   ├── GameRenderer.h
│   ├── TextManager.h
│   ├── ImageManager.h
│   ├── Socket.h
│   ├── AccountProtocol.h
│   ├── UserDB.h
│   └── utils.h
│
├── cards/                      # 卡牌图片资源（按颜色分子目录 + back.jpg）
│   ├── red/  blue/  green/  yellow/
│   ├── black/                  # 万能牌
│   └── pointer.jpg             # 手牌选择指针
│
├── characters/                 # 角色图片资源（每个角色一个子目录，含皮肤 .jpg）
│   ├── 特朗普/
│   ├── 李阳/
│   ├── 柯洁/
│   └── ……
│
└── users.json                  # 运行时生成：用户账号数据（自动创建）
```

---

## 🎴 卡牌规则

### 牌堆构成（`Pile::standard()`）

| 类别 | 颜色 | 牌名 | 每种数量 |
|---|---|---|---|
| 数字牌 | 红/蓝/绿/黄 | 0 | 3 张 |
| 数字牌 | 红/蓝/绿/黄 | 1\~9 | 各 4 张 |
| 功能牌 | 红/蓝/绿/黄 | 反转（Reverse） | 4 张 |
| 功能牌 | 红/蓝/绿/黄 | 封禁（Skip） | 4 张 |
| 功能牌 | 红/蓝/绿/黄 | +2（Draw2） | 5 张 |
| 万能牌 | 黑 | 变色（Wild） | 11 张 |
| 万能牌 | 黑 | +4（Wild Draw4） | 10 张 |

### 出牌规则（`Player::canUse()`）

打出一张牌需满足以下任一条件：

1. 当前无有效颜色（首张出牌）；
2. 牌的颜色与**当前颜色**相同；
3. 牌的牌名与**当前牌名**相同（同名牌可打出）；
4. 打出的是万能牌（变色 / +4）。

### 卡牌效果

| 牌名 | 效果 |
|---|---|
| **封禁 Skip** | 跳过目标玩家的下一回合 |
| **反转 Reverse** | 反转出牌方向（顺时针 ⇄ 逆时针） |
| **+2 Draw2** | 目标玩家摸 2 张牌并被封禁 |
| **变色 Wild** | 出牌者指定下一种有效颜色 |
| **+4 Wild Draw4** | 出牌者指定颜色，目标摸 4 张牌并被封禁；目标可**质疑** —— 若出牌者手牌中含当前颜色则质疑成功（出牌者改摸 4），否则质疑失败（目标摸 6 并被封禁） |

---

## 🧑‍🎤 角色技能系统

### 角色数据（`Character::infos`）

每名角色由 `Character::Info` 描述，包含：等级（S/A/B/C/D/F）、被动技能工厂列表、主动技能工厂列表、体力值（HP）。`Character::make()` 根据名称装配技能实例并初始化体力。

### 技能触发机制

- **被动技能（`PSkill`）**：绑定一个**触发时机**（`TriggerTime`，如 `game_begin` / `phase_begin` / `use_card_end` / `card_target_begin` 等 20+ 种）与**触发对象**（`TriggerPlayer`：self / others / anybody / nobody）。当 `GameLogic::launchPSkills()` 在对应时机被调用时，自动匹配并发动。
- **锁定技（forced）**：触发即自动发动，无需玩家确认。
- **限定技（limit=1）**：整局游戏仅可发动一次。
- **主动技能（`ASkill`）**：由玩家在出牌阶段主动选择发动。

### 技能触发时机一览

| 时机 | 说明 |
|---|---|
| `game_begin` / `game_end` | 一局游戏开始 / 结束时 |
| `phase_begin` / `phase_end` | 回合开始 / 结束时 |
| `phase_use1_begin/end` | 出牌阶段 1（摸牌前） |
| `phase_draw_begin/end` | 摸牌阶段 |
| `phase_use2_begin/end` | 出牌阶段 2（摸牌后） |
| `use_card_begin/end` | 使用牌时 / 后 |
| `lose_card_begin/end` | 失去牌时 / 后 |
| `card_target_begin/end` | 成为牌目标时 / 后 |
| `damage_begin/end` | 受到伤害时 / 后 |
| `recover_begin/end` | 回复体力时 / 后 |
| `draw_begin/end` | 摸牌时 / 后 |
| `discard_begin/end` | 弃牌时 / 后 |
| `recast_begin/end` | 重铸牌时 / 后 |
| `ban_begin/end` | 被封禁时 / 后 |
| `resolution_begin/end` | 决议时 / 后 |

### 部分角色示例

| 角色 | 等级 | HP | 技能 | 简介 |
|---|---|---|---|---|
| **棍母** | F | 110 | 隐身 | 锁定技：当你成为【+2】/【+4】的目标时，改为你的下家摸 1 张牌 |
| **Tralalero Tralala** | C | 160 | 耐克 | 锁定技：若上一张牌为蓝色或万能牌，【封禁】、【+2】和【+4】对你无效 |
| **Bombardiro Crocodilo** | A | 185 | 轰炸 | 锁定技：当你打出【+2】时，目标失去 2% 最大体力 |
| **Alan Walker** | C | 230 | 电音 / 蒙面 | 电音：回合开始时，你可令手牌中所有数字牌变成随机数字；蒙面：锁定技，你失去体力时数值减少 30%（向下取整） |
| **丁真** | C | 140 | 锐刻 | 当你打出【5】时，你可令一名角色摸 1 张牌；你可改为令其摸 5 张牌并失去此技能至本局结束 |
| **Brr Brr Patapim** | B | 235 | 森罗 / 大脚 | 森罗：锁定技，游戏开始时将手中所有非黑色牌变为绿色；大脚：回合开始时，可弃置一张万能牌并发动一次【森罗】 |
| **新关羽** | B | 210 | 过江 / 大盏 | 过江：锁定技，当你成为【+2】的目标时，来源摸 2 张牌；大盏：锁定技，回合开始时点数最小的数字牌均随机变大（至多变至 9），若全为【9】可将其中一张变为红色 |

> 完整角色列表与技能描述见 `Character::infos`（`Character.cpp`）与 `Skill.h` 中各技能类的构造函数。技能描述均直接取自源码，确保与运行行为一致。

---

## 🔄 游戏流程

### 服务端主循环（`ServerMain.cpp` → `GameLogic`）

```
启动服务器 (端口 8888)
    │
    ▼
等待两名客户端连接
    │
    ▼
初始化角色（config.json 指定 或 随机候选）
    │
    ▼
Ban 阶段（一号位禁用对方候选 → 二号位禁用）
    │
    ▼
Pick 阶段（一号位选角 → 二号位选角，可选皮肤）
    │
    ▼
进入游戏循环
    │
    ├── 当前玩家回合开始（phaseBegin）
    ├── 出牌阶段 1（phaseUse1）：可选择打出一张合法牌
    ├── 若未出牌 → 摸牌阶段（phaseDraw，巨富技能额外 +1）→ 出牌阶段 2（phaseUse2）
    ├── 回合结束（phaseEnd）
    ├── 切换至下一名玩家（altPlayer）
    └── 检查游戏是否结束（任一玩家 HP 归零）
            │
            ├── 未结束 → 继续下一回合
            └── 结束   → 双方按手牌分值扣血 → 检查 HP → 新一局 / 游戏终局
```

### 回合阶段详解（`Player::turn()`）

1. **封禁检查**：若本回合被封禁，跳过整个回合并解除封禁。
2. **phaseBegin**：回合开始触发技能（如「顶置」「军国」「买棋」「电音」「生存」「森罗」「大盏」等）。
3. **phaseUse1**：出牌阶段 1，玩家可选择打出一张合法牌（触发 `use_card_begin/end`、卡牌效果、技能链）。
4. **phaseDraw**：若未出牌，执行摸牌（默认 1 张，「巨富」为 2 张）。
5. **phaseUse2**：出牌阶段 2，再次允许出牌。
6. **phaseEnd**：回合结束触发技能（如「卖棋」「猥琐」）。

### 客户端主循环（`ClientMain.cpp`）

```
连接服务器 (IP/端口来自 config.json)
    │
    ▼
账号阶段：注册 / 登录（含用户名预检）
    │
    ▼
进入游戏主循环：
    ├── 处理窗口事件（按键 → 发送 ClientInput；鼠标点击 → 角色信息框）
    ├── 接收服务器包（GameState / Choice / GameStart / GameEnd / ConnectionInfo）
    ├── 更新并渲染游戏画面（60 FPS）
    └── 显示选项提示框（标题/选项/倒计时/分页）
```

---

## 🌐 网络通信协议

### 消息类型（`MessageType`，`Socket.h`）

| 枚举值 | 方向 | 说明 |
|---|---|---|
| `ClientInput` | C → S | 客户端按键输入（`sf::Keyboard::Scancode` + `playerId`） |
| `GameState` | S → C | 完整游戏状态快照（`GameState` 序列化） |
| `ConnectionInfo` | S → C | 分配 `playerId` + 就绪状态 |
| `ConnectionRefused` | S → C | 服务器已满，拒绝连接 |
| `GameStart` | S → C | 游戏开始通知 |
| `GameEnd` | S → C | 游戏结束，附带胜者 ID |
| `Choice` | S → C | 向指定客户端弹出选项提示框（标题/选项/是否强制/超时/分页） |
| `RegisterRequest/Response` | C → S / S → C | 注册请求与响应（含积分/胜场/负场） |
| `LoginRequest/Response` | C → S / S → C | 登录请求与响应 |
| `CheckUsernameRequest/Response` | C → S / S → C | 用户名是否已存在预检 |

### 状态同步（`GameState`）

服务器每回合通过 `GameLogic::broadcastState()` 向两名客户端各发送一份 `GameState`：

- **当前玩家索引** / **当前颜色** / **当前牌名** / **出牌方向** / **座次表**
- **每名玩家的**：ID / 角色名 / 皮肤 / 当前 HP / 最大 HP / 手牌（**仅自己的手牌为真实牌面，对手手牌以背面牌填充**）/ 选中牌索引
- **弃牌堆**（全部牌面可见，用于 +4 质疑判定等）

### 账号协议（`AccountProtocol`）

- 注册：客户端发送 `RegisterRequest(username, password)` → 服务端 `UserDB::registerUser()` 校验唯一性并落盘 → 返回 `AccountResponse(ok, msg, points, wins, losses)`。
- 登录：客户端发送 `LoginRequest(username, password)` → 服务端 `UserDB::login()` 校验密码 → 返回账号信息；登录成功后自动进入游戏等待。
- 用户名预检：注册时先发送 `CheckUsernameRequest` 检查用户名是否已被占用。

### 用户数据库（`UserDB`）

- 单例模式，底层为 `std::unordered_map<std::string, UserInfo>`（`users_`），通过 `nlohmann/json` 序列化至 `users.json` 文件持久化。
- `UserInfo` 含 `password` / `points` / `wins` / `losses`。
- 游戏结束时 `addMatchResult()` 按 `unool::scoreboard[胜者等级][败者等级]` 查表加分并立即落盘。

---

## 🚀 构建与运行

### 环境要求

- **操作系统**：Windows 10/11（代码中使用 `<Windows.h>` 设置控制台 UTF-8 代码页）
- **IDE / 编译器**：Visual Studio 2026（MSVC，语言标准设为 `/std:c++latest` 即 C++23）
- **依赖库**：[SFML 3.x](https://www.sfml-dev.org/)（Graphics / Network / System / Window modules）、[nlohmann/json](https://github.com/nlohmann/json)（单头文件 `json.hpp`）
- **网络库**：Windows Socket（`ws2_32.lib`，Visual Studio 默认提供）

### SFML 配置步骤（Visual Studio 2026）

1. 下载 [SFML 3.x](https://www.sfml-dev.org/download.php) Windows SDK 版本并解压到本地（如 `C:/SFML-3.0`）。
2. 在 Visual Studio 中打开 `UNOOL.sln`，右键项目 → **属性**：
   - **VC++ 目录 → 包含目录**：添加 `C:/SFML-3.0/include`
   - **VC++ 目录 → 库目录**：添加 `C:/SFML-3.0/lib`
   - **链接器 → 输入 → 附加依赖项**：添加以下 lib（Debug 配置加 `-d` 后缀）：
     ```
     sfml-graphics.lib
     sfml-window.lib
     sfml-system.lib
     sfml-network.lib
     ws2_32.lib
     ```
3. 将 SFML 的 DLL 文件（位于 `C:/SFML-3.0/bin`）复制到输出目录（`x64/Debug/` 或 `x64/Release/`）。
4. 将 `nlohmann/json` 的 `json.hpp` 放入项目包含路径（如 `header/json.hpp` 或任意 include 目录）。

### 构建步骤

1. 在 Visual Studio 2026 中打开 `UNOOL.sln`。
2. 选择配置：**Debug**（开发调试）或 **Release**（发布构建）。
3. 菜单栏 → **生成 → 生成解决方案**（或按 `Ctrl+Shift+B`）。
4. 产物位于 `x64/Debug/UNOOL.exe` 或 `x64/Release/UNOOL.exe`。

> **提示**：确保运行时 `cards/`、`characters/`、`users.json`（首次运行自动创建）、`config.json` 位于可执行文件所在目录或其父目录（`ImageManager` 以可执行文件父目录为资源根路径 `UNOOL`）。

### 配置说明（`config.json`）

```json
{
  "server":   { "ip": "127.0.0.1", "port": 8888 },
  "size": {
    "window":   { "width": 2000, "height": 1200 },
    "card":     { "width": 180,  "height": 270 },
    "pointer":  { "width": 90,   "height": 135 },
    "character":{ "width": 315,  "height": 405 }
  },
  "candidateCount": 3,
  "initHandCount": 7
}
```

| 字段 | 说明 |
|---|---|
| `server.ip` / `server.port` | 客户端连接的目标服务器地址（服务端默认监听 8888） |
| `size.window` | 客户端渲染窗口尺寸 |
| `size.card` / `size.pointer` / `size.character` | 卡牌 / 选择指针 / 角色立绘的显示尺寸 |
| `candidateCount` | 每名玩家的候选角色数量（Ban/Pick 阶段） |
| `initHandCount` | 初始手牌数（「巨富」角色会覆盖为 12） |
| `characters` | （可选）若指定，则双方直接使用该数组中的两个角色名，跳过随机候选与 Ban/Pick |

### 运行方式

1. **启动服务端**：在 Visual Studio 中将 Server 项目设为启动项目（`右键 → 设为启动项目`），按 `F5` 运行；或直接在输出目录运行 `Server.exe`，等待两名客户端连接。
2. **启动客户端**：将 Client 项目设为启动项目，按 `F5` 运行；或直接在输出目录运行 `Client.exe`，按提示输入服务器 IP 与端口（或通过 `--title` / `-t` 命令行参数指定窗口标题）。
3. **账号操作**：客户端连接后进入账号系统，选择「注册」或「登录」。
4. **开始游戏**：两名玩家均登录后，服务端自动进入 Ban/Pick 阶段与游戏循环。

---

## 🎹 客户端操作说明

| 按键 | 功能 |
|---|---|
| `←` / `→` | 在手牌中左右选择 |
| `Space` | 按颜色+牌名排序手牌 |
| `↑` | 确认打出当前选中的牌 / 确认选择 |
| `↓` | 取消（在非强制选项中可用） |
| 数字键 `0`\~`9` / 小键盘 | 在选项提示框中选择对应编号选项 |
| 鼠标点击角色立绘 | 弹出/关闭该角色信息框（等级/HP/技能描述） |

---

## 📝 开发说明

### 新增角色

1. 在 `Character.cpp` 的 `Character::infos` 中添加新条目：`{角色名, {等级, {被动技能工厂列表}, {主动技能工厂列表}, HP}}`。
2. 在 `characters/角色名/` 目录下放入皮肤图片（默认皮肤命名为 `默认.jpg`，可放多张 `.jpg` 供皮肤选择）。
3. 若需新技能，在 `Skill.h` 中继承 `PSkillImpl<派生类>` 或 `ASkillImpl<派生类>` 定义新技能类，在 `Skill.cpp` 中实现 `filter()` 与 `content()`，并在角色 info 中挂接其工厂 `派生类::make`。

### 新增卡牌

- 在 `Card.h` 的 `Card::Name` 枚举中添加新牌名。
- 在 `Card.cpp` 的 `Card::imagePaths` 中注册对应图片路径。
- 若为功能/万能牌，在 `Effect.cpp` 中添加对应效果函数，并在 `Card::applyEffect()` 中分派。

### 代码约定

- 使用 `std::unique_ptr` 管理卡牌/角色/玩家等堆对象的所有权；跨模块引用优先使用 `std::reference_wrapper`（`ref<T>` / `opt_ref<T>` 别名）。
- 所有跨网络传输的结构体均通过 `sf::Packet` 的 `operator<<` / `operator>>` 序列化。
- 中文字符串统一使用 `std::wstring` 传输与渲染，工具函数 `unool::string::to_utf16()` / `to_utf8()` 在 UTF-8 与 UTF-16 间转换。

---

## 📄 License

本项目为课程/个人学习项目，仅供交流与学习使用。角色名称与图片资源版权归各自原作者所有，卡牌规则灵感来源于经典 UNO 游戏。
