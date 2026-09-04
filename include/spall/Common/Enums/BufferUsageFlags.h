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
		Vertex = SPALL_BIT(0),
		Index = SPALL_BIT(1),
		Uniform = SPALL_BIT(2),
		TransferSource = SPALL_BIT(3),
		TransferDestination = SPALL_BIT(4),
		Storage = SPALL_BIT(5),
		Indirect = SPALL_BIT(6),
		AccelerationStructureInput = SPALL_BIT(7)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		BufferUsageFlags)
} // namespace spall
