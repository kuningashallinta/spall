// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies the operations for which a texture is created.
	enum class TextureUsageFlags : std::uint32_t
	{
		None = 0,
		ColorAttachment = SPALL_BIT(0),
		DepthStencilAttachment = SPALL_BIT(1),
		TransferSource = SPALL_BIT(2),
		TransferDestination = SPALL_BIT(3),
		Sampled = SPALL_BIT(4),
		Storage = SPALL_BIT(5)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		TextureUsageFlags)
} // namespace spall
