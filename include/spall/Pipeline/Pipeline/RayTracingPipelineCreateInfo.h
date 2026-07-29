#pragma once

#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/PushConstantInfo.h>
#include <spall/Pipeline/Pipeline/RayTracingHitGroup.h>
#include <spall/Pipeline/Shader/PipelineShaderStageInfo.h>

#include <cstdint>
#include <span>

namespace spall
{
	/// Every referenced stage must name its entry point, and the declared sizes
	/// must cover every shader in the pipeline.
	struct RayTracingPipelineCreateInfo
	{
		PipelineShaderStageInfo RayGenerationShader = {};

		std::span<const PipelineShaderStageInfo> MissShaders;
		std::span<const RayTracingHitGroup> HitGroups;

		/// Largest ray payload, in bytes, any shader declares.
		std::uint32_t MaxPayloadSize = 0;

		/// Largest hit attribute, in bytes. Built-in triangle attributes are eight.
		std::uint32_t MaxAttributeSize = 8;

		/// Deepest TraceRay nesting, counting the ray-generation invocation as one.
		std::uint32_t MaxRecursionDepth = 1;

		std::span<const IResourceSetLayout* const> ResourceSetLayouts;
		PushConstantInfo PushConstants = {};
	};
} // namespace spall
