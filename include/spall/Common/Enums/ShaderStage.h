// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	enum class ShaderStage
	{
		Vertex,
		Fragment,
		Compute,
		Geometry,
		TessellationControl,
		TessellationEvaluation,
		RayGeneration,
		Miss,
		ClosestHit,
		AnyHit,
		Intersection
	};
} // namespace spall
