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
