// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	enum class ShaderStageFlags : std::uint32_t
	{
		None = 0,
		Vertex = SPALL_BIT(0),
		Fragment = SPALL_BIT(1),
		Compute = SPALL_BIT(2),
		Geometry = SPALL_BIT(3),
		TessellationControl = SPALL_BIT(4),
		TessellationEvaluation = SPALL_BIT(5),
		RayGeneration = SPALL_BIT(6),
		Miss = SPALL_BIT(7),
		ClosestHit = SPALL_BIT(8),
		AnyHit = SPALL_BIT(9),
		Intersection = SPALL_BIT(10)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		ShaderStageFlags)
} // namespace spall
