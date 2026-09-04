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
		Red = SPALL_BIT(0),
		Green = SPALL_BIT(1),
		Blue = SPALL_BIT(2),
		Alpha = SPALL_BIT(3),
		All = SPALL_BIT(0) | SPALL_BIT(1) | SPALL_BIT(2) | SPALL_BIT(3)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		ColorComponentFlags)
} // namespace spall
