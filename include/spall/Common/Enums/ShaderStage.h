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
