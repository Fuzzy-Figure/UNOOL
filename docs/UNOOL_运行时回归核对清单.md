# UNOOL 运行时回归核对清单（任务 #73）

> 目的：确认六边形分层重构（阶段 1–4，提交 `cfa91f3` → `37cd2ae`）**未改变运行时行为与线上协议字节**。
> 本清单补「运行时」这一环——代码层面已静态验证，此处验证「跑起来之后」。

> ✅ **验证结果（2026-11-10）**：用户已按本清单在本地 `Debug|x64` 与 `Release|x64` 双配置、Client1/Client2 双开同服实跑，**全部 8 个分区、约 30 项核验均通过，无失败项**。任务 #73 据此闭环。字节级等价方面：原方案 §110 要求的「录制/回放设施」未建，故以「`Socket.cpp:617` 序列化源码行未变 + 行为等价」作为静态保证，与清单第 7 节说明一致。

## 0. 前置与判定约定

- **静态已验证（无需重测）**：协议 / 端口文件 git diff 为空——`MessageType` 25 值/顺序、端口 8888/14341、`sf::Packet` 序列化、`server_config.json` / `client_config.json`、`PacketDispatcher` / `IClientGateway` / `IClientSession` / `ICommands.h` / `ShopData.h`、`Socket.cpp` 全部函数体在本轮零改动；10 个 XML 全解析；Client1/Client2 条目 diff 0。
- **环境要求**：`Debug|x64` 与 `Release|x64` **各跑一遍**；`Client1` 与 `Client2` **同时**连接同一 `Server`；服务端用同工程产物。
- **判定**：每项为 `[ ] 通过 / [ ] 失败`。失败需记录**现象 + 截图/日志**，并定位到对应提交决定是否回退。
- **字节级说明**：原方案 §110 要求的「录制/回放设施」本轮**未建**，故真正的逐字节等价需用抓包基线对比（见第 7 节），否则以「序列化源码行未变 + 行为等价」为静态保证。

## 1. 编译与启动（S1–S4 通用）

- [ ] `Debug|x64` Rebuild：0 错误 0 警告，Server / Client1 / Client2 三 exe 均生成
- [ ] `Release|x64` Rebuild：0 错误 0 警告
- [ ] Client1 与 Client2 可**同时**连接同一 Server，互不影响

## 2. 登录链路（S3a / S3b1 / S3b2 风险点）

- [ ] 查重 → 注册 → 登录**同帧连续发包**，顺序与重构前一致（`LoginScene::pollAccountPackets` 同帧触发，见提交 `ea7293c` / `4543897`）
- [ ] **跨阶段留包**：非账号类包（游戏包）不被消费、留在队列交给 `gamePhase`——`next(net, {5 类订阅})` Filtered 语义（`LoginScene.cpp:246`）
- [ ] 登录成功回大厅、状态机分支（`Mode` / `Status` / `Focus`）与全部文案不变
- [ ] 签到弹窗与积分同步显示正确

## 3. 收包分发与跨阶段留包（S2 风险点，提交 `6f9e71b`）

- [ ] `gamePhase` 每帧 `next(net)`（DrainAll）消费所有可达包（`ClientApp.cpp:98`）
- [ ] `LoginScene::pollAccountPackets` 仅消费账号类，其余留队
- [ ] `ShopScene::handleNetwork` 仅处理商城/技能/购买/心跳（`ShopScene.cpp:54`）
- [ ] 三处调用点**全部经 `PacketDispatcher`**，无遗漏的直连 socket 收包

## 4. 心跳与 3 秒超时（S2 风险点）

- [ ] `replyHeartbeat` 在 `PacketDispatcher::next` 内**统一回复**（定义 `PacketDispatcher.cpp:13`，调用 :27 / :43；原 gamePhase / LoginScene / ShopScene 三处心跳回复已收拢至此）
- [ ] idle 客户端每 ~3 秒收到心跳回包，不误判掉线
- [ ] `Socket.cpp:387`「loggedIn 客户端连续 3 秒未收到任何回包视为掉线」触发条件与重构前一致
- [ ] 连接 / 重连 3 秒超时（`Socket.cpp:575` `sf::seconds(3)`）不变；接收 1ms 轮询（`Socket.cpp:601`）不变

## 5. 商城流程（S3c 风险点，最复杂，提交 `6fd96e9`）

- [ ] 进入商城：组合根 `ClientApp` 构造并运行 `ShopScene`，返回大厅时序不变（`IShopOpener::openShop`）
- [ ] **商城数据刷新时序**：进入即拉 `ShopData`，分页 / 搜索 / 展开后按需补请求
- [ ] **技能描述缓存命中**：`skillsCache`（进程级 static，`ShopScene.cpp:12`）退出再进仍命中、省一次请求（`ShopScene.cpp:818` 命中分支）
- [ ] **`pendingSkills` 去重**：重复角色名只请求一次（`ShopScene.cpp:43` `contains` / `:45` `insert` / `:87` `erase` / `:823`）
- [ ] **购买结果提示文案四态**：
  - [ ] `"购买成功"`（`ShopScene.cpp:104`，绿色 `ShopScene.cpp:527`）
  - [ ] `"积分不足！"`（`result.msg` 空时，`ShopScene.cpp:108`）
  - [ ] `"商城数据解析失败"`（`ShopScene.cpp:76`）
  - [ ] `"购买结果解析失败"`（`ShopScene.cpp:99`）
- [ ] 购买成功后服务端同步刷新商城数据给该客户端（`ShopService.cpp:327`）

## 6. 输入适配（S4 风险点，字节级，提交 `37cd2ae`）

- [ ] **`ClientInput.key` 线上字节逐字一致**：`ClientNetwork::sendClientInput`（`Socket.cpp:617`：`packet << static_cast<int>(key) << playerId << selectedIndex`，`key = keyEvent->scancode`）源码行未变 → 字节不变
- [ ] 键位映射：Left/A → `movePointerLeft`、Right/D → `movePointerRight`、其他键 → `sendClientInput(selectedIndex)`（`InputAdapter.cpp:24/27/31`）
- [ ] `canSelectLocal()` 判定不变；鼠标左键 → `handleMouseClick` 不变
- [ ] 关闭窗口（Closed）→ 退出 `gamePhase` 语义不变（`pollAndDispatch()` 返回 `false` 驱动 `return`）

## 7. 协议字节不变的总校验（推荐但可选）

- **若有录制/回放设施**（原方案 §110，本轮未建）：新旧客户端对同输入序列产出字节流 diff 应为 0。
- **若无**：在 `Socket.cpp:617` 前后用临时日志打印 `sf::Packet` 字节，与**重构前抓包基线**对比（需你保存一份重构前快照）。
- **静态兜底**：`MessageType.h`、两个 config、端口常量、所有 `send* / <<` 序列化调用在本轮零改动（各提交说明已登记）。

## 8. 收尾

- [ ] 上述全绿 → 任务 #73 标记完成，回填至方案第 56 节 / README / 工作记忆
- [ ] 任一失败 → 记录现象，定位到对应提交（`cfa91f3`/`6f9e71b`/`d8699a2`/`ea7293c`/`4543897`/`6fd96e9`/`37cd2ae`），决定是否回退或修复

---

*清单起草：2026-11-10，对应任务 #73（方案 52.8 / 49.7）。覆盖方案第 55.7、56.6 节所列回归项，并补全 S4 输入字节级核验。*

---

## 9. 退出匹配功能（2026-11-10 新增）

> 需求：点击「开始游戏」后可退出匹配，避免另一名玩家不点开始导致永远等待。
> 协议新增 `CancelMatchmaking` / `MatchmakingCancelled`，追加在 `MessageType` 末尾，既有 25 个枚举值与线上字节不变。
> 按后续需求移除「退出匹配」相关的一次性文字提示：退出后静默回大厅，仅保留服务端日志与按钮 / 等待界面的状态文案。`MatchNoticeState::pendingMessage` 与客户端 `parseMatchmakingCancelled` 随提示一并移除；`MatchmakingCancelled` 广播仍带账号名字段，枚举值与线上字节不变。

- [ ] `Debug|x64` / `Release|x64` Rebuild：0 错误 0 警告（本次改动均在既有编译单元内，无需改 vcxproj）
- [ ] **单方等待**：Client1 点「开始游戏」→ 显示「正在匹配对手...」和已等待秒数；Client2 不点，Client1 不再跳到空白游戏画面
- [ ] **按钮 / Esc 退出**：Client1 点「退出匹配」或按 Esc → 静默回到大厅，不显示「已退出匹配 / 已返回大厅」等文字提示；服务端打印 `退出匹配，已重置双方准备状态`
- [ ] **对手通知**：双方都在匹配等待时一方退出 → 另一方也静默回大厅，不显示「对手已退出匹配」文字提示
- [ ] **重新匹配**：回大厅后任一方再次点「开始游戏」，双方都点后正常开局（验证 `playerReady` 已清空，无残留准备状态）
- [ ] **开局竞态**：双方几乎同时点开始、其中一方立刻点退出 → 以服务端 `GameStart` 为准进入游戏，不出现一方在大厅、一方在游戏
- [ ] **超时兜底**：模拟服务端不回解散包 → 客户端 3 秒后自动回大厅（无文字提示）
- [ ] **跨阶段留包**：`GameStart` 仅 peek 不消费，`gamePhase` 仍打印「游戏开始！」并正常渲染
- [ ] **断线**：匹配等待中关闭服务端 → 客户端结束会话，不空等
- [ ] **ReadyToPlay 修复验证**：双方都点「开始游戏」确实开局（此前服务端把只有消息号的 ReadyToPlay 交给 `parseAccountRequest` 解析失败后整包丢弃，双方都点了也不会开局）
- [ ] **回归**：登录 / 注册 / 签到 / 商城 / 心跳 3 秒超时链路不受影响

---

## 10. 对手开始匹配提示（2026-11-10 新增）

> 需求：一名玩家点击「开始游戏」后，另一名玩家应收到提示。
> 协议新增 `MatchmakingStarted`，追加在 `MessageType` 末尾；既有 25 个线上枚举值与第 9 节新增的 2 个值均不变。

- [ ] `Debug|x64` / `Release|x64` Rebuild：0 错误 0 警告（`IShopOpener::openShop` 签名、ShopScene 构造参数的全部调用点已同步）
- [ ] **大厅提示**：Client1 点「开始游戏」→ Client2 大厅出现黄色横幅「对手「账号」已开始匹配，请点击「开始游戏」加入」，控制台打印同文案
- [ ] **不误显**：发起方自己的匹配等待界面不出现横幅；坏包 / 账号名等于自己时忽略
- [ ] **提示消除**：Client1 退出匹配 → Client2 横幅消失，不显示退出匹配文字提示
- [ ] **加入匹配**：Client2 看到横幅后点「开始游戏」→ 双方立即开局（横幅清除，GameStart 仍只 peek 不消费）
- [ ] **晚登录 / 重连**：Client1 先点开始、Client2 尚未登录 → Client2 登录后仍收到补发提示；匹配期间重连同样能看到
- [ ] **商城场景**：Client2 在商城时 Client1 点开始 → 商城内出现深色横幅，退出商城后大厅横幅仍在；商城期间对手退出匹配 → 商城横幅消失、退出商城后大厅也不残留任何退出匹配提示
- [ ] **对手掉线**：匹配期间关闭 Client1 → Client2 收到解散广播，横幅消失 / 匹配等待界面自动回大厅，不空等
- [ ] **不刷屏**：重复 ReadyToPlay 包不会重复广播（仅「未准备→已准备」跃迁发一次）；解散后重新匹配会再次通知
- [ ] **回归**：第 9 节全部用例、登录 / 注册 / 签到 / 商城 / 心跳 3 秒超时链路不受影响
