# UNOOL — UNO Online

**UNOOL** = **UNO** + **OnLine**：基于 C++23 / SFML 3.x 的双人联机卡牌对战游戏。用标准 UNO 牌堆做载体，自创角色技能与体力规则（更接近三国杀的玩法框架）。

- **70 个可选角色**，每个带 1～2 个技能；**130 个技能类**（104 被动 / 22 即时 / 4 转换）
- **C/S 架构**，服务端权威；玩家只能看到自己的手牌
- 账号注册/登录 + 按角色等级差的积分系统

## 技术栈

| 项 | 选型 |
|---|---|
| 语言 | C++23（MSVC `/std:c++latest`） |
| 图形 / 网络 | SFML 3.x（Graphics / Network / Window / System） |
| JSON | nlohmann/json（单头文件） |
| 构建 | Visual Studio 2026（`.slnx`） |
| 平台 | Windows 10/11 |

## 构建与运行

### 前置准备（仓库不含这些）

1. **SFML 3.x** — 下载后修改三个 `.vcxproj` 中的 `AdditionalIncludeDirectories` / `AdditionalLibraryDirectories`（当前硬编码为作者本机路径，需改成本机路径）
2. **nlohmann/json** — 把 `json.hpp` 放入任意 include 目录
3. **美术资源** — `cards/`（按颜色分目录）与 `characters/`（每角色一个目录，含 `默认.jpg`）**已被 `.gitignore` 排除**，需自备

### 运行

三个项目：`Server`（服务端）、`Client1` / `Client2`（同一份 `ClientMain.cpp` 的两份拷贝，用于本地双开）。

1. 先启动 `Server`，监听 **8888** 端口（硬编码于 `ServerMain.cpp`），等待两个客户端登录
2. 再启动两个 Client，在 `client_config.json` 中填好服务端 IP
3. 两端都登录后自动进入 Ban/Pick 与游戏循环

> **工作目录敏感**：配置读取 `../client_config.json`、`../server_config.json`，资源根为可执行文件的父目录。请在 `x64/Debug/` 或 `x64/Release/` 下启动。

## 配置

`client_config.json`

```json
{
  "fonts": "C:/Windows/Fonts/msyh.ttc",
  "server": { "ip": "127.0.0.1", "port": 8888 },
  "size": { "window": {...}, "card": {...}, "pointer": {...}, "character": {...} }
}
```

`server_config.json`

| 字段 | 说明 |
|---|---|
| `mode` | `normal`（单人一角）/ `double`（双将，每家 2 个角色合成） |
| `singleCandidateCount` / `doubleCandidateCount` | 候选角色数 |
| `singleInitHandCount` / `doubleInitHandCount` | 初始手牌数 |
| `banCount` | Ban/Pick 阶段每人禁用次数 |
| `characters` | 可选。指定则跳过随机候选与 Ban/Pick（normal 给 2 个、double 给 4 个） |
| `shielded` | 屏蔽不参与随机的角色名 / 分组 |

## 玩法

- **牌堆**：四色 0～9 + 反转 / 封禁 / +2 + 变色 / +4，标准 UNO 构成
- **出牌**：无有效颜色（首张）／同色／同名／万能牌
- **体力**：回合结束时手牌分值之和转为伤害，体力归零即败北
- **+4 可质疑**：出牌者手牌含原色则质疑成功（出牌者摸 4），否则质疑失败（目标摸 6 并被封禁）
- **技能**：被动技按触发时机自动发动，锁定技无需确认，限定技每局一次；主动技在出牌阶段按数字键发动

### 操作

| 按键 | 功能 |
|---|---|
| `←` `→` / `A` `D` | 左右选牌 |
| `Space` | 按颜色+牌名排序 |
| `↑` / `W` | 确认出牌 / 确认选项 |
| `↓` / `S` | 取消（非强制选项） |
| `0`~`9` | 选项编号 / 发动主动技 |
| 鼠标点击立绘 | 查看角色信息 |

## 目录结构

```
header/     18 个头文件
source/     20 个源文件（ClientMain / ServerMain 为两端入口）
Client1/ Client2/ Server/   三个 VS 项目
userDatas.json              运行时生成的用户数据
```

核心模块：`GameLogic`（回合/座次/牌堆/技能触发）、`Player`（出牌摸牌与交互）、`Card`（牌堆与手牌容器）、`Skill` 系（技能框架）、`GameRenderer`（客户端渲染）、`Socket`（网络封装）。

## 扩展

**加角色**：在 `Character.cpp` 的 `Character::infos` 加一行 `{角色名, {分组, 等级, {被动技工厂}, {主动技工厂}, HP}}`，并在 `characters/角色名/` 放置 `默认.jpg`。

**加技能**：继承 `PassiveSkillImpl<X>` / `InstantSkillImpl<X>` / `TransformSkillImpl<X>`，实现 `filter()` 与 `content()`，在角色 info 中挂 `X::make`。

## 已知限制

- 无单元测试、无 CI
- 密码明文存储于 `userDatas.json`
- 客户端异常断线会导致服务端阻塞
- 仅支持 Windows

## License

课程/个人学习项目，仅供交流学习。角色名称与图片资源版权归各自原作者所有，卡牌规则灵感来源于经典 UNO。
