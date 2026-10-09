# 积分商城系统 Spec

## Why
当前用户积分仅用于比赛胜负加减，缺乏消费渠道。需要完整的积分商城系统，让用户通过积分购买英雄、皮肤、道具，使积分具有实际意义，同时引入皮肤品质效果和道具系统丰富游戏策略。

## What Changes

### 用户数据模型变更
- **BREAKING**: `UserInfo` 结构体新增 `ownedCharacters`（拥有的角色名集合）、`ownedSkins`（拥有的皮肤 map: 角色名→皮肤名集合）、`items`（道具 map: 道具名→数量）、`characterPool`（单人模式可选角色名列表）
- `userDatas.json` 格式变更，新增上述字段

### 登录界面变更
- 登录界面用户名输入框上方新增"积分商城"按钮，左右与用户名框对齐
- 点击按钮进入商城界面，不影响正常登录流程

### 新增商城界面（客户端）
- 全新 SFML 场景，包含英雄列表和道具列表两个视图
- 英雄列表按 Level F→S 排列，每页10个，居中靠左
- 选中英雄后展开皮肤列表
- 右上角显示剩余积分，左上角显示操作提示
- 搜索功能（按 S 键）、界面切换（E/H 键）
- 购买失败提示"积分不足！"

### 角色选择流程变更
- **BREAKING**: 单人模式选将改为从用户 `characterPool` 中随机抽取5个候选（双方不重复），正常进行 ban 环节
- 初始角色池由管理员配置（fuzzyfigure、lazer 用户指定初始角色）
- 除测试用户1、2外，所有用户现有皮肤设为不可选

### 皮肤品质系统
- 新增皮肤品质枚举：普通、精品、史诗、传说、至尊、饮料限定
- 不同品质有不同售价和对战胜利积分加成
- 饮料限定皮肤：集齐该角色所有其他皮肤后自动获得

### 道具系统
- 选将扩充卡：购买后可选英雄数量+1（双模式通用），初始88积分，每次购买后售价+100×已购买次数，每人至多购买5次，购买5次后从商城消失
- 增分卡：即时道具，进入战斗前使用（双方禁/选完将后，后手先用），胜利则积分翻倍（可与满血叠加翻四倍），战败则无事发生，售价10积分

### 英雄售价表
- F档：棍母/夏搏/电棍=488；幺幺/蒋介石=288；其余=20
- D档：blueberrini/octopussini=188；其余=40
- C档：斯大林=288；金日成/唐伯虎=128；其余=88
- B档：弗兰肯/黑鸦=188；新开头角色=168；其余=148
- A档：均为228
- S档：无法正常购买；集齐除S外一个档位所有角色→随机解锁其中一个S档角色；若ABCDF均已集齐→可488积分购买S档角色

### 比赛结算变更
- `addMatchResult` 新增皮肤品质加成逻辑
- 增分卡效果在结算时触发

## Impact
- Affected code: UserDB.h/cpp, Socket.h/cpp, LoginScene.cpp, ClientMain.cpp, GameRenderer.cpp, GameLogic.cpp, Character.h/cpp, ServerMain.cpp, 新增 ShopScene.cpp/h, 新增 ShopData.h
- Affected data: userDatas.json 格式变更
- Affected config: server_config.json 可能需要新增皮肤品质配置

## ADDED Requirements

### Requirement: 用户数据模型扩展
UserInfo 结构体 SHALL 新增以下字段：
- `std::set<std::string> ownedCharacters`：已解锁角色名集合
- `std::map<std::string, std::set<std::string>> ownedSkins`：已解锁皮肤（角色名→皮肤名集合）
- `std::map<std::string, int> items`：道具数量（道具名→数量）
- `std::vector<std::string> characterPool`：单人模式可选角色名列表

#### Scenario: 新用户注册
- WHEN 用户注册新账号
- THEN ownedCharacters 为空，ownedSkins 为空，items 为空，characterPool 为空（需管理员配置初始角色池）

#### Scenario: 已有用户数据迁移
- WHEN 程序加载旧格式 userDatas.json
- THEN 缺失的新字段使用默认空值，不报错

### Requirement: 初始角色池配置
系统 SHALL 为 fuzzyfigure 和 lazer 用户配置指定初始角色池。

#### Scenario: fuzzyfigure 初始角色
- fuzzyfigure 的 characterPool 为：霍金,科比,大章鱼,白羊座,天蝎座,特朗普,双子座,王耘浩,alanwalker,植物人

#### Scenario: lazer 初始角色
- lazer 的 characterPool 为：二次元,虎哥,柯尔特,狮子座,天蝎座,田淑丽,格斯,丁真,薛维旭,艾尔普利莫

#### Scenario: 测试用户保留
- 测试用户1和测试用户2不受皮肤不可选限制，保留所有现有皮肤

### Requirement: 皮肤不可选状态
系统 SHALL 将除测试用户1、2外的所有用户的英雄皮肤设为不可选状态（从 ownedSkins 中移除）。

#### Scenario: 执行皮肤重置
- WHEN 管理员执行皮肤重置操作
- THEN 除测试用户1、2外，所有用户的 ownedSkins 被清空，ownedCharacters 被清空，characterPool 被设置为指定的初始角色

### Requirement: 积分商城入口
登录界面 SHALL 在用户名输入框上方显示"积分商城"按钮。

#### Scenario: 进入商城
- WHEN 用户在登录界面点击"积分商城"按钮
- THEN 进入商城界面，显示英雄列表

#### Scenario: 返回登录
- WHEN 用户在商城界面点击返回或按 ESC
- THEN 返回登录界面

### Requirement: 商城英雄列表
商城英雄列表 SHALL 按 Level F→S 排列，每页10个，居中靠左显示。

#### Scenario: 浏览英雄
- GIVEN 用户进入商城英雄界面
- THEN 显示所有角色列表，按 F→D→C→B→A→S 排列
- AND 每页显示10个，可通过键盘翻页
- AND 每项显示角色名和等级标识

#### Scenario: 选中英雄查看皮肤
- WHEN 用户选择一个英雄
- THEN 展开该英雄的皮肤列表
- AND 列表首位为"英雄（未解锁/已解锁）（售价）"，已解锁则不显示第二个括号
- AND 从第二位开始显示皮肤（默认皮肤不算），皮肤后括号显示解锁状态和售价

### Requirement: 商城右上角积分显示
商城任何界面 SHALL 在右上角显示用户当前剩余积分。

#### Scenario: 积分实时更新
- WHEN 用户购买成功
- THEN 右上角积分更新为扣除后的值

#### Scenario: 积分不足提示
- WHEN 用户尝试购买超出当前积分的物品
- THEN 购买失败，弹出"积分不足！"提示

### Requirement: 商城搜索功能
商城内按 S 键 SHALL 弹出搜索栏，支持中文字符串匹配。

#### Scenario: 搜索英雄
- WHEN 用户按 S 键并输入关键词
- THEN 匹配角色名中包含该关键词的英雄，显示匹配结果

### Requirement: 商城界面切换
商城内按 E 键切换到道具界面，按 H 键切换回英雄界面。

#### Scenario: 切换到道具
- WHEN 用户按 E 键
- THEN 显示道具列表

#### Scenario: 切换到英雄
- WHEN 用户按 H 键
- THEN 显示英雄列表

### Requirement: 商城左上角操作提示
商城任何界面 SHALL 在左上角显示"S搜索，E/H切换界面"。

### Requirement: 道具列表
道具界面 SHALL 显示可用道具及其数量和售价。

#### Scenario: 选将扩充卡显示
- GIVEN 用户未购买满5次选将扩充卡
- THEN 道具列表显示"选将扩充卡（当前数量：X）（售价）"
- AND 售价为 88 + 100 × 已购买次数

#### Scenario: 选将扩充卡购买上限
- WHEN 用户已购买5次选将扩充卡
- THEN 该道具从商城消失

#### Scenario: 增分卡显示
- THEN 道具列表显示"增分卡（当前数量：X）（10）"

### Requirement: 选将扩充卡效果
购买选将扩充卡后，用户单人模式可选英雄数量+1（双模式通用）。

#### Scenario: 购买扩充卡
- WHEN 用户购买选将扩充卡
- THEN 下次游戏选将时候选数量+1
- AND 道具数量不变（购买即生效，非消耗品）

### Requirement: 增分卡使用
增分卡为即时道具，在双方均禁/选完将后使用，后手先用。

#### Scenario: 使用增分卡
- GIVEN 双方禁/选完将
- WHEN 后手玩家有增分卡
- THEN 后手玩家先选择是否使用增分卡
- AND 先手玩家随后选择是否使用增分卡
- AND 使用后消耗一张增分卡

#### Scenario: 增分卡胜利效果
- WHEN 使用了增分卡的玩家胜利
- THEN 获得积分翻倍（×2），可与满血翻倍叠加（×4）
- AND 增分卡效果不受皮肤品质加成影响

#### Scenario: 增分卡失败效果
- WHEN 使用了增分卡的玩家失败
- THEN 无事发生，不额外扣分

### Requirement: 皮肤品质系统
皮肤 SHALL 分为6种品质，不同品质有不同售价和对战效果。

#### Scenario: 普通品质
- 售价8积分，无特殊效果

#### Scenario: 精品品质
- 售价28积分，对战胜利时获得积分+1（不受翻倍影响）

#### Scenario: 史诗品质
- 售价48积分，对战胜利时获得积分+2（不受翻倍影响）

#### Scenario: 传说品质
- 售价88积分，对战胜利时获得积分+20%（在各种翻倍效果后触发，向上取整）

#### Scenario: 至尊品质
- 售价188积分，对战结束时无论胜负获得5积分

#### Scenario: 饮料限定品质
- 集齐该角色所有其他皮肤后自动获得，不售卖
- 穿戴时每次打出万能牌后获得1积分

### Requirement: 英雄售价
英雄售价 SHALL 按等级和角色名区分。

#### Scenario: F档售价
- 棍母/夏搏/电棍=488积分；幺幺/蒋介石=288积分；其余F档=20积分

#### Scenario: D档售价
- blueberrini/octopussini=188积分；其余D档=40积分

#### Scenario: C档售价
- 斯大林=288积分；金日成/唐伯虎=128积分；其余C档=88积分

#### Scenario: B档售价
- 弗兰肯/黑鸦=188积分；新开头角色=168积分；其余B档=148积分

#### Scenario: A档售价
- 所有A档=228积分

#### Scenario: S档解锁
- S档角色无法直接购买
- 集齐除S外一个档位的所有角色→随机解锁其中一个S档角色（解锁时弹出窗口）
- 若ABCDF均已集齐→可488积分购买S档角色

### Requirement: 单人模式选将改造
单人模式选将 SHALL 从用户 characterPool 中随机抽取候选，双方不重复。

#### Scenario: 生成候选
- WHEN 单人模式开始选将
- THEN 从玩家1的 characterPool 中随机抽取5个候选（加上扩充卡数量）
- AND 从玩家2的 characterPool 中随机抽取5个候选（加上扩充卡数量）
- AND 双方候选无重复（排除对方已抽取的角色）
- AND 正常进行 ban 环节

#### Scenario: 角色池不足
- IF 玩家角色池不足以生成所需候选数量
- THEN 从全部角色中补抽（排除已选和已ban的）

### Requirement: 比赛结算皮肤效果
addMatchResult SHALL 根据玩家穿戴的皮肤品质增加额外积分。

#### Scenario: 精品皮肤胜利
- WHEN 穿戴精品皮肤的玩家胜利
- THEN 在基础积分（含翻倍）后额外+1

#### Scenario: 史诗皮肤胜利
- WHEN 穿戴史诗皮肤的玩家胜利
- THEN 在基础积分（含翻倍）后额外+2

#### Scenario: 传说皮肤胜利
- WHEN 穿戴传说皮肤的玩家胜利
- THEN 在基础积分（含翻倍）后额外+20%（向上取整）

#### Scenario: 至尊皮肤任意结果
- WHEN 穿戴至尊皮肤的玩家对战结束
- THEN 无论胜负获得+5积分

#### Scenario: 饮料限定皮肤万能牌
- WHEN 穿戴饮料限定皮肤的玩家打出万能牌
- THEN 获得1积分（实时更新到 UserDB）

### Requirement: 选将扩充卡售价递增
选将扩充卡售价 SHALL 初始88积分，每次购买后售价+100×已购买次数。

#### Scenario: 首次购买
- WHEN 用户首次购买选将扩充卡
- THEN 售价为88积分

#### Scenario: 第二次购买
- WHEN 用户已购买1次，再次购买
- THEN 售价为88+100×1=188积分

#### Scenario: 第五次购买
- WHEN 用户已购买4次，再次购买
- THEN 售价为88+100×4=488积分，购买后该道具从商城消失
