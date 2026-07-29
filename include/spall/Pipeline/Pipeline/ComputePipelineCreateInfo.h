#pragma once

#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/PushConstantInfo.h>
#include <spall/Pipeline/Shader/PipelineShaderStageInfo.h>

#include <span>

namespace spall
{
	/// Describes a compute pipeline.
	///
	/// The referenced shader and layouts must belong to the creating device.
	struct ComputePipelineCreateInfo
	{
		PipelineShaderStageInfo ComputeShader = {};

		std::span<const IResourceSetLayout* const> ResourceSetLayouts;
		PushConstantInfo PushConstants = {};
	};
} // namespace spall
