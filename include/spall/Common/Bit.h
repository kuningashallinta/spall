// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <type_traits>

#define SPALL_BIT(x) (1u << (x))

#define SPALL_ENUM_CLASS_BITWISE_OPERATORS(type)                                                          \
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
	constexpr inline type operator^(type left, type right)                                                \
	{                                                                                                     \
		using UnderlyingType = std::underlying_type_t<type>;                                              \
		return static_cast<type>(static_cast<UnderlyingType>(left) ^ static_cast<UnderlyingType>(right)); \
	}                                                                                                     \
	constexpr inline type operator~(type value)                                                           \
	{                                                                                                     \
		using UnderlyingType = std::underlying_type_t<type>;                                              \
		return static_cast<type>(~static_cast<UnderlyingType>(value));                                    \
	}                                                                                                     \
	inline type& operator|=(type& left, type right)                                                       \
	{                                                                                                     \
		return left = left | right;                                                                       \
	}                                                                                                     \
	inline type& operator&=(type& left, type right)                                                       \
	{                                                                                                     \
		return left = left & right;                                                                       \
	}                                                                                                     \
	inline type& operator^=(type& left, type right)                                                       \
	{                                                                                                     \
		return left = left ^ right;                                                                       \
	}
