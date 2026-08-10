// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	enum class TextureAspectFlags : std::uint32_t
	{
		None = 0,
		Color = BIT(0),
		Depth = BIT(1),
		Stencil = BIT(2)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		TextureAspectFlags)
} // namespace spall
