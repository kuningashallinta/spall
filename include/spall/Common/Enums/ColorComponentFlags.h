// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	enum class ColorComponentFlags : std::uint32_t
	{
		None = 0,
		Red = BIT(0),
		Green = BIT(1),
		Blue = BIT(2),
		Alpha = BIT(3),
		All = BIT(0) | BIT(1) | BIT(2) | BIT(3)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		ColorComponentFlags)
} // namespace spall
