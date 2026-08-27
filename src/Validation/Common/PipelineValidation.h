#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/Pipeline/ComputePipelineCreateInfo.h>
#include <spall/Pipeline/Pipeline/PipelineCreateInfo.h>
#include <spall/Pipeline/Pipeline/RayTracingPipelineCreateInfo.h>
#include <src/Validation/Common/FormatValidation.h>
#include <src/Validation/Common/TextureValidation.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace spall
{
	inline Status validateResourceSetLayoutList(std::span<const IResourceSetLayout* const> resourceSetLayouts);

	inline Status validatePushConstantInfo(
		const PushConstantInfo& info,
		ShaderStageFlags supportedStages);

	inline Status validateRasterizerState(const PipelineCreateInfo& info);

	inline Status validateBlendState(const PipelineCreateInfo& info);

	inline Status validateShaderStages(const PipelineCreateInfo& info);

	inline Status validateColorTargets(const PipelineCreateInfo& info);

	inline Status validateDepthState(const PipelineCreateInfo& info);

	inline Status validateVertexLayout(const PipelineCreateInfo& info);

	inline Status validatePipelineCreateInfo(const PipelineCreateInfo& info);

	inline Status validateComputePipelineCreateInfo(const ComputePipelineCreateInfo& info);

	inline Status validateRayTracingShaderStage(const PipelineShaderStageInfo& stage);

	inline Status validateRayTracingPipelineCreateInfo(const RayTracingPipelineCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/PipelineValidation.inl>
