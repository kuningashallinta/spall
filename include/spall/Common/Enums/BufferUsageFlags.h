// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies the operations for which a buffer is created.
	enum class BufferUsageFlags : std::uint32_t
	{
		None = 0,
		Vertex = BIT(0),
		Index = BIT(1),
		Uniform = BIT(2),
		TransferSource = BIT(3),
		TransferDestination = BIT(4),
		Storage = BIT(5),
		Indirect = BIT(6),
		AccelerationStructureInput = BIT(7)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		BufferUsageFlags)
} // namespace spall
