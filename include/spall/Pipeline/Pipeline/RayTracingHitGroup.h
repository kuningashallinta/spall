// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Pipeline/Shader/PipelineShaderStageInfo.h>

namespace spall
{
	/// Describes one hit group of a ray-tracing pipeline.
	///
	/// At least one stage must be present. A group with an intersection shader
	/// handles procedural bounding-box geometry; one without handles triangles.
	struct RayTracingHitGroup
	{
		PipelineShaderStageInfo ClosestHitShader = {};
		PipelineShaderStageInfo AnyHitShader = {};
		PipelineShaderStageInfo IntersectionShader = {};
	};
} // namespace spall
