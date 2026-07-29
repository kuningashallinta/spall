#pragma once

#include <type_traits>

#define BIT(x) (1u << (x))

#define ENUM_CLASS_BITWISE_OPERATORS(type)                                                                \
	constexpr inline type operator|(type left, type right)                                                \
	{                                                                                                     \
		using UnderlyingType = std::underlying_type_t<type>;                                              \
		return static_cast<type>(static_cast<UnderlyingType>(left) | static_cast<UnderlyingType>(right)); \
	}                                                                                                     \
	constexpr inline type operator&(type left, type right)                                                \
	{                                                                                                     \
		using UnderlyingType = std::underlying_type_t<type>;                                              \
		return static_cast<type>(static_cast<UnderlyingType>(left) & static_cast<UnderlyingType>(right)); \
	}                                                                                                     \
	inline type& operator|=(type& left, type right)                                                       \
	{                                                                                                     \
		return left = left | right;                                                                       \
	}
