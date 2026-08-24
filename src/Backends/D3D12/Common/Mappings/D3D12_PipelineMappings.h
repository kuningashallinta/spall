// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Pipeline/Pipeline/BlendStateInfo.h>
#include <spall/Pipeline/Pipeline/StencilFaceStateInfo.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	inline D3D12_PRIMITIVE_TOPOLOGY primitiveTopology(
		PrimitiveTopology topology,
		std::uint32_t patchControlPoints);

	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType(
		PrimitiveTopology topology);

	inline D3D12_CULL_MODE cullMode(
		CullMode mode);

	inline D3D12_FILL_MODE fillMode(
		FillMode mode);

	inline BOOL frontCounterClockwise(
		FrontFace frontFace)
	{
		return (frontFace == FrontFace::CounterClockwise) ? TRUE : FALSE;
	}

	inline D3D12_BLEND blendFactor(
		BlendFactor factor);

	inline D3D12_BLEND_OP blendOp(
		BlendOp op);

	inline D3D12_COMPARISON_FUNC compareOp(
		CompareOp op);

	inline D3D12_STENCIL_OP stencilOp(
		StencilOp op);

	inline UINT8 colorComponentFlags(
		ColorComponentFlags mask);

	inline D3D12_RENDER_TARGET_BLEND_DESC blendState(
		const BlendStateInfo& info);

	inline D3D12_DEPTH_STENCILOP_DESC stencilFaceDescription(
		const StencilFaceStateInfo& state);

	inline D3D12_SHADER_VISIBILITY shaderStageFlags(
		ShaderStageFlags stages);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_PipelineMappings.inl>
