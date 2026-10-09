# Tasks

## Phase 1: 用户数据模型与初始数据

- [x] Task 1: 扩展 UserInfo 结构体
  - [x] SubTask 1.1: 在 UserDB.h 的 UserInfo 中新增 ownedCharacters(set<string>), ownedSkins(map<string,set<string>>), items(map<string,int>), characterPool(vector<string>) 字段
  - [x] SubTask 1.2: 更新 NLOHMANN_DEFINE_TYPE_INTRUSIVE 宏包含新字段
  - [x] SubTask 1.3: 确保旧格式 userDatas.json 加载时缺失字段使用默认空值

- [x] Task 2: 初始化用户数据
  - [x] SubTask 2.1: 编写一次性初始化函数：清空所有用户（除测试用户1、2）的 ownedCharacters 和 ownedSkins
  - [x] SubTask 2.2: 设置 fuzzyfigure 的 characterPool 为：霍金,科比,大章鱼,白羊座,天蝎座,特朗普,双子座,王耘浩,alanwalker,植物人
  - [x] SubTask 2.3: 设置 lazer 的 characterPool 为：二次元,虎哥,柯尔特,狮子座,天蝎座,田淑丽,格斯,丁真,薛维旭,艾尔普利莫
  - [x] SubTask 2.4: 将上述初始角色加入相应用户的 ownedCharacters

## Phase 2: 皮肤品质与售价系统

- [x] Task 3: 定义皮肤品质枚举与售价表
  - [x] SubTask 3.1: 在 Character.h 或新建 SkinData.h 中定义 SkinQuality 枚举（普通/精品/史诗/传说/至尊/饮料限定）
  - [x] SubTask 3.2: 定义皮肤品质售价映射（普通8/精品28/史诗48/传说88/至尊188/饮料限定不售卖）
  - [x] SubTask 3.3: 建立皮肤目录与品质的映射规则（可配置或按目录名约定）

- [x] Task 4: 英雄售价表
  - [x] SubTask 4.1: 在 Character.h/cpp 中新增 getCharacterPrice(name) -> int 静态方法
  - [x] SubTask 4.2: 实现按等级和角色名的售价查找逻辑（F/D/C/B/A/S 各档不同价格）

## Phase 3: 商城数据与网络通信

- [x] Task 5: 商城数据结构与服务器端接口
  - [x] SubTask 5.1: 新增 ShopData.h，定义商城所需的数据结构（角色列表带解锁状态、皮肤列表带解锁状态和品质、道具列表）
  - [x] SubTask 5.2: 在 ServerNetwork 中新增商城相关网络包类型（ShopDataRequest/ShopDataResponse/BuyRequest/BuyResponse）
  - [x] SubTask 5.3: 在 ServerNetwork 中新增发送商城数据和处理购买请求的方法
  - [x] SubTask 5.4: 在 UserDB 中新增购买角色(purchaseCharacter)、购买皮肤(purchaseSkin)、购买道具(purchaseItem)方法，含积分校验

- [x] Task 6: S档角色解锁逻辑
  - [x] SubTask 6.1: 在购买角色时检查是否集齐某档位所有角色，若是则随机解锁一个S档角色
  - [x] SubTask 6.2: 若ABCDF均已集齐，开放S档角色488积分购买
  - [x] SubTask 6.3: 返回解锁信息供客户端弹窗显示

## Phase 4: 客户端商城UI

- [x] Task 7: 新建 ShopScene 场景
  - [x] SubTask 7.1: 新建 ShopScene.h/cpp，继承或参考 LoginScene 的 SFML 场景模式
  - [x] SubTask 7.2: 实现商城主循环（事件处理+渲染），支持 ESC 返回登录
  - [x] SubTask 7.3: 实现英雄列表渲染（F→S排列，每页10个，居中靠左，翻页）
  - [x] SubTask 7.4: 实现选中英雄后展开皮肤列表（首位英雄项+后续皮肤项，括号显示解锁状态和售价）
  - [x] SubTask 7.5: 实现右上角积分显示和左上角操作提示
  - [x] SubTask 7.6: 实现购买交互（选择未解锁物品→确认购买→发送请求→处理结果，失败弹"积分不足！"）

- [x] Task 8: 商城搜索功能
  - [x] SubTask 8.1: 按 S 键弹出搜索栏
  - [x] SubTask 8.2: 实现中文字符串匹配，显示匹配结果列表

- [x] Task 9: 商城道具界面
  - [x] SubTask 9.1: 按 E 键切换到道具界面，按 H 键切回英雄界面
  - [x] SubTask 9.2: 实现道具列表渲染（选将扩充卡/增分卡，显示当前数量和售价）
  - [x] SubTask 9.3: 选将扩充卡购买5次后从列表消失

## Phase 5: 登录界面变更

- [x] Task 10: 登录界面添加商城入口
  - [x] SubTask 10.1: 在 LoginScene 的用户名输入框上方添加"积分商城"按钮（左右与用户名框对齐）
  - [x] SubTask 10.2: 点击按钮时创建 ShopScene 并运行，返回后恢复登录界面

## Phase 6: 选将流程改造

- [x] Task 11: 单人模式选将从用户角色池抽取
  - [x] SubTask 11.1: 在 GameLogic 的 initPlayersNormal 中，改为从双方用户的 characterPool 随机抽取候选（默认5+扩充卡数量）
  - [x] SubTask 11.2: 确保双方候选无重复（先抽玩家1，再从玩家2角色池排除玩家1已抽角色后抽取）
  - [x] SubTask 11.3: 角色池不足时从全角色补抽
  - [x] SubTask 11.4: 保留现有 ban 环节

## Phase 7: 道具系统

- [x] Task 12: 选将扩充卡逻辑
  - [x] SubTask 12.1: 在选将时根据用户 items["选将扩充卡"] 增加候选数量
  - [x] SubTask 12.2: 售价递增逻辑（88+100×已购买次数），5次后消失

- [x] Task 13: 增分卡使用流程
  - [x] SubTask 13.1: 双方禁/选完将后，后手先选择是否使用增分卡
  - [x] SubTask 13.2: 先手随后选择是否使用增分卡
  - [x] SubTask 13.3: 使用后消耗一张，记录使用状态供结算时判定

## Phase 8: 比赛结算皮肤效果

- [x] Task 14: addMatchResult 新增皮肤品质加成
  - [x] SubTask 14.1: 查询胜者穿戴皮肤品质，按品质加成额外积分（普通0/精品+1/史诗+2/传说+20%向上取整/至尊+5无论胜负）
  - [x] SubTask 14.2: 增分卡效果：胜利积分×2，可与满血×2叠加为×4
  - [x] SubTask 14.3: 至尊皮肤败者也+5
  - [x] SubTask 14.4: 饮料限定皮肤打出万能牌+1积分（在 PassiveSkill 中触发）

## Phase 9: 集成与测试

- [x] Task 15: 客户端-服务器商城通信联调
  - [x] SubTask 15.1: 客户端打开商城时请求商城数据
  - [x] SubTask 15.2: 购买请求发送和响应处理
  - [x] SubTask 15.3: 积分实时同步更新

- [x] Task 16: 端到端流程验证
  - [x] SubTask 16.1: 登录→进入商城→购买英雄→返回登录→开始游戏→选将使用新英雄
  - [x] SubTask 16.2: 购买皮肤→游戏中穿戴→胜利后积分加成验证
  - [x] SubTask 16.3: 购买增分卡→使用→胜利翻倍验证
  - [x] SubTask 16.4: 购买选将扩充卡→选将数量+1验证
  - [x] SubTask 16.5: 集齐档位→S档解锁验证

# Task Dependencies
- Task 2 depends on Task 1
- Task 4 depends on Task 3
- Task 5 depends on Task 1, Task 3, Task 4
- Task 6 depends on Task 5
- Task 7 depends on Task 5
- Task 8 depends on Task 7
- Task 9 depends on Task 7
- Task 10 depends on Task 7
- Task 11 depends on Task 1
- Task 12 depends on Task 1
- Task 13 depends on Task 1
- Task 14 depends on Task 3, Task 13
- Task 15 depends on Task 7, Task 5
- Task 16 depends on all previous tasks
