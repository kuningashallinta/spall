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
		TriangleCullDisable = BIT(0),
		TriangleFlipFacing = BIT(1),
		ForceOpaque = BIT(2),
		ForceNonOpaque = BIT(3)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		AccelerationStructureInstanceFlags)
} // namespace spall
