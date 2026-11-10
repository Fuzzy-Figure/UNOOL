#pragma once
// B+ 共享契约层：角色 / 皮肤相关的纯值类型。
//
// 约束（独立编译的最低条件）：
//   * 只依赖标准库；不 include 任何 domain / application / adapters 头；
//   * 不依赖 PCH、不依赖 SFML / json.hpp / Windows.h；
//   * 不链接规则引擎（无成员函数定义、无静态数据、无 IO）。
// 协议 DTO 与未来的安卓客户端外壳只需包含本头即可读写角色的等级 / 品质 / 价格，
// 无需把 Character.h -> Skill.h -> 179 个技能类的头文件闭包带进编译单元。
//
// Character.h 通过类型别名复用这里的定义，因此既有代码中所有
// Character::Level / Character::price_t / Character::hp_t 写法保持源码兼容。
#include <compare>
#include <cstddef>
#include <format>
#include <optional>
#include <string_view>
#include <type_traits>

namespace unool::contract {

	using hp_t = int;
	// std::nullopt 表示不可购买（S 档角色、饮料限定皮肤等）
	using price_t = std::optional<std::size_t>;

	enum class CharacterLevel { F, D, C, B, A, S };

	// 顺序即品质高低，bestSkinQuality() 依赖 q > best 的比较，不得重排。
	enum class SkinQuality {
		normal, fine, epic, legend, supreme, drink
	};

	inline constexpr std::strong_ordering
		operator<=>(const CharacterLevel a, const CharacterLevel b) noexcept {
		using underlying = std::underlying_type_t<CharacterLevel>;
		return static_cast<underlying>(a) <=> static_cast<underlying>(b);
	}

	inline constexpr std::string_view to_string(const CharacterLevel level) noexcept {
		switch (level) {
			case CharacterLevel::F: return "F";
			case CharacterLevel::D: return "D";
			case CharacterLevel::C: return "C";
			case CharacterLevel::B: return "B";
			case CharacterLevel::A: return "A";
			case CharacterLevel::S: return "S";
			default:                return "?";
		}
	}

	inline constexpr std::string_view to_string(const SkinQuality quality) noexcept {
		switch (quality) {
			case SkinQuality::normal:  return "普通";
			case SkinQuality::fine:    return "精品";
			case SkinQuality::epic:    return "史诗";
			case SkinQuality::legend:  return "传说";
			case SkinQuality::supreme: return "至尊";
			case SkinQuality::drink:   return "饮料限定";
			default:                   return "未知";
		}
	}
}

// 与重构前 Character.h 中的特化等价：别名指向同一类型，格式化输出不变。
template<>
struct std::formatter<unool::contract::CharacterLevel> : std::formatter<std::string_view> {
	auto format(const unool::contract::CharacterLevel level, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(
			unool::contract::to_string(level), ctx);
	}
};
