# UNOOL — UNO Online

基于 C++23 / SFML 3.1 的双人联机卡牌对战游戏：以标准 UNO 牌堆为载体，叠加自创角色技能、体力与 Ban/Pick 规则。服务端权威，每人只看到自己的真实手牌。

- **123 名可选角色 / 238 个技能类**（构建期从代码生成，见 [`docs/characters.md`](docs/characters.md)）
- 账号注册 / 登录（口令 PBKDF2-HMAC-SHA256 加盐存储）+ 按角色等级差查表的积分系统
- C/S 架构，仅 x64 / Windows 10·11

> 完整玩法规则以 [`docs/rules.md`](docs/rules.md) 为唯一事实源；当前架构与落地状态见 [`CURRENT.md`](CURRENT.md)。
> 阶段 0~4 的历史批次日志与旧 ADR 已归档到 [`docs/adr/`](docs/adr/)，不参与现状判断。

## 构建与运行

1. 用 Visual Studio 2026 打开 `UNOOL.slnx`，选 **x64**（Debug|x64 或 Release|x64），Ctrl+Shift+B 生成。**仅 x64**——Win32 配置为 C++20，无法通过编译。
2. 把 `dep/SFML/bin` 下的 .dll 复制到 .exe 同目录（`x64/Debug/` 或 `x64/Release/`）。
3. 先启动 `Server`（监听 **8888**，硬编码于 `ServerMain.cpp`），再启动 `Client`。多开客户端用同一 exe 启动多次，玩家座位由服务端分配。

> 仓库 `client_config.json` 指向作者公网地址与 **14341** 端口，与服务端 8888 不同，这是内网穿透映射的**预期配置**，不是缺陷；本机直连把 `server.port` 改成 8888 即可。不要统一端口或加一致性校验。

客户端连接参数优先级为 命令行 > 环境变量 > 配置文件：

| 来源 | 服务端地址 | 端口 | 存档路径 |
|---|---|---|---|
| 命令行 | `--ip <addr>` | `--port <n>` | — |
| 环境变量 | `UNOOL_SERVER_IP` | `UNOOL_SERVER_PORT` | `UNOOL_USERDATA` |
| 配置文件 | `client_config.json` 的 `server.ip` | 同上 `server.port` | 资源根 `userDatas.json` |

> 路径定位不依赖工作目录：`exeDir()` 取 exe 自身目录，`locate()` / `resourceRoot()` 向上查找配置与资源；VS 启动、双击 exe、任意 CWD 命令行启动结果一致。

## 配置

`client_config.json`

| 字段 | 说明 |
|---|---|
| `fonts` | 字体文件路径 |
| `server.ip` / `server.port` | 连接的服务端地址；默认 14341（穿透映射），服务端监听恒为 8888 |
| `size.window` / `card` / `pointer` / `character` | 窗口、卡牌、指针、立绘显示尺寸 |

`server_config.json`

| 字段 | 说明 |
|---|---|
| `mode` | `normal`（每家 1 角色）/ `double`（双将合成） |
| `characters.assign` | 可选，指定角色则跳过随机候选与 Ban/Pick |
| `characters.shielded.characters` / `.groups` | 屏蔽不参与抽选的角色 / 分组 |
| `rules.normal` / `rules.double`.`candidateCount` / `initHandCount` | 各模式候选数与初始手牌数 |
| `rules.banCount` | Ban/Pick 每人禁用对方次数 |
| `network.reconnectTimeoutSec` | 断线重连等待秒数（当前 600） |

## 玩法速览

标准 UNO 牌堆 + 自创技能：同色 / 同名 / 万能牌出牌，+4 可质疑；每局结束按手牌分值扣体力，归零败北；被动技按触发时机自动发动，主动技出牌阶段按数字键发动。Ban/Pick 拼点定座次后互 Ban 选角。完整规则（含体力扣减、摸牌来源、生命上限、技能触发顺序、积分查表）见 [`docs/rules.md`](docs/rules.md)。

| 按键 | 功能 |
|---|---|
| `←` `→` / `A` `D` | 左右选牌 |
| `Space` | 按颜色+牌名排序手牌 |
| `↑` / `W` | 确认出牌 / 确认选项 |
| `↓` / `S` | 取消（非强制选项） |
| `0`~`9` / 小键盘 | 选项编号 / 发动主动技 / 翻页 |
| 鼠标点击立绘 | 查看角色信息（等级/HP/标记/封印/技能） |

## 扩展

- **加角色**：在 `source/core/Character.cpp` 的 `Character::infos` 加一行，并在 `images/characters/<分组>/<角色名>/默认.jpg` 放置立绘（分组名须与目录一致）。改完运行 `python tools/gen_stats.py` 刷新角色表统计。
- **加技能**：继承 `PassiveSkillImpl<X>` / `InstantSkillImpl<X>` / `TransformSkillImpl<X>` 并标记 `final`，实现 `filter()` / `content()`，在角色 info 中挂接。新增 `.cpp` 放进对应目录即可——无需手改任何工程文件，运行 `python tools/verify_structure.py --generate` 同步条目。
- **新增 `.cpp` 到共享目录 `source/shell/`**：须在 `tools/projects.manifest.json` 的对应工程 `compile.files` 登记一次（唯一事实源），再跑生成器。
- **日志与格式化**：新代码统一用 `std::format` / `std::println`；`Card::Color`、`Card::Name`、`Character::Level` 已特化 `std::formatter`。

## 工程结构

`UNOOL.slnx` 含 4 个工程：`Server`、`Client`（可执行）+ `UNOOL.Adapters`、`UNOOL.Core`（静态库）。仅 x64。`Core` 统一编译规则内核、协议与用例服务；`Adapters` 编译服务端适配器；平台实现由 `Projects/PlatformShell.props` 在 2 个 exe 中共用。新增源文件不手改工程文件——生成器以 `tools/projects.manifest.json` 为事实源，`--generate` 写条目、`--check` 阻止漂移。

## 文档

- [`CURRENT.md`](CURRENT.md) — 当前架构与落地状态（现状唯一入口）
- [`docs/rules.md`](docs/rules.md) — 游戏规则唯一事实源
- [`docs/characters.md`](docs/characters.md) — 自动生成的角色表与统计
- [`docs/adr/`](docs/adr/) — 历史批次日志与旧 ADR 归档（不参与现状判断）
- `tools/verify_structure.py` — 结构校验 + 工程文件生成器（`--check` / `--generate`）
- `tools/gen_stats.py` — 角色 / 技能统计生成器（`--check`）

## 截图

待补（需实际运行游戏截屏）。现仅有素材图：`images/cards/`、`images/characters/`、`images/marks/`。

## 已知限制

- 服务端全程单线程且阻塞，`ask()` / `chooseCard()` 同步等待客户端输入；同时只服务一桌。
- 客户端断线后重连等待默认 600 秒；非阻塞 socket 的 `Partial` 重发可能破坏协议流。
- 子技能强耦合父技能：父技能被移除后子技能仍可能触发并抛异常。
- 协议为破坏性换代（Arch-C），旧版客户端 / 服务端互不兼容；版本握手在登录包完成。
- 仅支持 Windows / x64；无单元测试与 CI（见 `CURRENT.md` 的待办）。

## License

MIT，见 [`LICENSE`](LICENSE)。第三方依赖与素材许可见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。角色名称与图片版权归各自原作者；玩法灵感来源于经典 UNO。
