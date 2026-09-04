// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies how one top-level instance overrides the geometry it references.
	///
	/// ForceOpaque and ForceNonOpaque are mutually exclusive.
	enum class AccelerationStructureInstanceFlags : std::uint32_t
	{
		None = 0,
		TriangleCullDisable = SPALL_BIT(0),
		TriangleFlipFacing = SPALL_BIT(1),
		ForceOpaque = SPALL_BIT(2),
		ForceNonOpaque = SPALL_BIT(3)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		AccelerationStructureInstanceFlags)
} // namespace spall
