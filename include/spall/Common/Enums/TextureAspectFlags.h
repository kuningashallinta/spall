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
		Color = SPALL_BIT(0),
		Depth = SPALL_BIT(1),
		Stencil = SPALL_BIT(2)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		TextureAspectFlags)
} // namespace spall
