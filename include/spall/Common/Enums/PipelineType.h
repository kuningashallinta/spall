#pragma once

namespace spall
{
	/// Identifies which kind of work a pipeline records.
	enum class PipelineType
	{
		Graphics,
		Compute,
		RayTracing
	};
} // namespace spall
