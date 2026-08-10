// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Limits.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>
#include <spall/Pipeline/Pipeline/BlendStateInfo.h>
#include <spall/Pipeline/Pipeline/PushConstantInfo.h>
#include <spall/Pipeline/Pipeline/StencilFaceStateInfo.h>
#include <spall/Pipeline/Shader/PipelineShaderStageInfo.h>
#include <spall/Pipeline/VertexInput/VertexAttributeInfo.h>
#include <spall/Pipeline/VertexInput/VertexBindingInfo.h>
#include <spall/RenderPass/RenderPassBeginInfo.h>

#include <cstdint>
#include <span>

namespace spall
{
	/// Describes a graphics pipeline and its fixed-function state.
	///
	/// Spans and referenced objects must remain valid only for the duration of
	/// createPipeline.
	struct PipelineCreateInfo
	{
		PipelineShaderStageInfo VertexShader = {};
		PipelineShaderStageInfo FragmentShader = {};
		PipelineShaderStageInfo GeometryShader = {};
		PipelineShaderStageInfo TessellationControlShader = {};
		PipelineShaderStageInfo TessellationEvaluationShader = {};

		std::span<const VertexBindingInfo> VertexBindings;
		std::span<const VertexAttributeInfo> VertexAttributes;

		/// Resource-set layouts in binding order.
		///
		/// The D3D12 backend flattens each set into b, t, and s registers using
		/// the highest binding of every preceding layout.
		std::span<const IResourceSetLayout* const> ResourceSetLayouts;

		PrimitiveTopology PrimitiveTopology = PrimitiveTopology::TriangleStrip;
		std::uint32_t PatchControlPoints = 0;
		FillMode FillMode = FillMode::Solid;
		CullMode CullMode = CullMode::None;
		FrontFace FrontFace = FrontFace::Clockwise;
		std::int32_t DepthBias = 0;
		float DepthBiasClamp = 0.0f;
		float SlopeScaledDepthBias = 0.0f;
		float LineWidth = 1.0f;

		Format ColorTargetFormats[MaxColorAttachments] = {};
		std::uint32_t ColorTargetFormatCount = 0;
		Format DepthStencilFormat = Format::Unknown;
		std::uint32_t SampleCount = 1;

		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		CompareOp DepthCompareOp = CompareOp::LessOrEqual;

		bool EnableStencilTest = false;
		StencilFaceStateInfo FrontStencilState = {};
		StencilFaceStateInfo BackStencilState = {};

		/// Masks and reference are shared by the front and back stencil faces.
		std::uint8_t StencilReadMask = 0xff;
		std::uint8_t StencilWriteMask = 0xff;
		std::uint8_t StencilReference = 0;

		/// Blend state corresponding to each color-target format.
		BlendStateInfo BlendStates[MaxColorAttachments] = {};

		PushConstantInfo PushConstants = {};
	};
} // namespace spall
