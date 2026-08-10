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
		Vertex = BIT(0),
		Fragment = BIT(1),
		Compute = BIT(2),
		Geometry = BIT(3),
		TessellationControl = BIT(4),
		TessellationEvaluation = BIT(5),
		RayGeneration = BIT(6),
		Miss = BIT(7),
		ClosestHit = BIT(8),
		AnyHit = BIT(9),
		Intersection = BIT(10)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		ShaderStageFlags)
} // namespace spall
