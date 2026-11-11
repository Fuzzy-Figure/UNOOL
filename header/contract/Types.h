#pragma once
#include <cstddef>

// 领域层的最小公共类型。
// 第一阶段只抽出 player_id；现有领域类（Card / Player / GameLogic / Skill 系）
// 保持在全局命名空间，避免本阶段对 4400+ 行技能实现产生任何改动。
namespace unool::contract {
	using player_id = std::size_t;
}
