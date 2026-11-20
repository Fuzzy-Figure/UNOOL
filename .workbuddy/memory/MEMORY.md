# UNOOL 项目长期约定

- 写完不跑自检（编译语法自检、`verify_structure.py`、`git diff --check`、编码换行检查），直接交付；用户反馈自检太慢。
- 初始化统一用 `{}`；禁用非右值裸指针（`this` / 第三方库交互除外）；所有权只用智能指针，观察只用引用/`reference_wrapper`。
- 新增角色技能优先复用既有分组目录的 `Passive.h/.cpp`、`Instant.h/.cpp`，不动 `.vcxproj` / `.filters`。
- 枚举扩展（`Color::purple`、`Name::secret_*`、`DrawReason::card_draw*` 等）一律追加在枚举末尾，保证线上序列化数值不变。
- 中文标识符在本项目可正常使用（已有大量中文类名/技能名），MSVC 与 GCC 均可编译。
- **契定技** = 初始非锁定技（构造 `forced=false`），发动一次后 `setForced(true)` 变锁定技，`reset()` 里 `setForced(false)` 每局重置；先例在「网络」分组（举报 / 爆缸 / 落水）。
- 被动技次数可动态增加：`increaseLimit(n)`；`limit=0` 即「每局限零次」，`Skill::reset()` 每局把 limit 恢复为构造值。
- 座位顺序 = `game.getPlayers()` 的数组顺序（`nextPlayerIndex` 按该数组推进）；轮末用 `round_end` + `TriggerPlayer::anybody`（`TriggerPlayer::self` 只在自己是轮末玩家时触发）。

## 秘技牌约定
- `Card::Name` 用 **`secret_中文名`**：`secret_毒酒` / `secret_药酒` / `secret_过载`。
- `Effect` 静态函数同样用中文名：`Effect::毒酒` / `Effect::药酒`（`Effect::过载` 无效果，不可打出）。
- 卡图路径：`images/cards/secret/中文名.jpg`，即 `毒酒.jpg` / `药酒.jpg` / `过载.jpg`。
- `value()` 统一返回 **10**（计入败者手牌价值）。
- `Card::Type` 有独立 `secret` 类型，`getType()` 对 `is_secret()` 的牌返回 `Type::secret`，`to_string(Type)` 返回「秘技牌」。
- 秘技牌不进标准牌堆与 `getAllCards()` 随机牌池，只由角色技能发放。
- 玩家判等一律用 `Player::operator==` / `!=`，不要用 `.getId()` 比较；`getId()` 只用于按 id 查询、容器键、协议/日志字段。


- 「绝地技」= 每场（每局）限一次的限定技，文案保留「绝地技」；先例邦尼【发射】（用户 2026-11-15 确认）。
- `draw_begin` 的 `Trigger.source` 现表示「使该角色摸牌的出牌者」：`Effect::draw2/draw4` 经 `Player::draw` 第4参数 `opt_ref<Player>` 传入，默认 `std::nullopt`，其它摸牌不受影响；【炮击】据此区分「自己打出的牌让他人摸牌」。
- `Character::setMaxHp()` / `Player::setMaxHp()`：改生命上限并把当前体力压到新上限，供【发射】减上限至 75。
