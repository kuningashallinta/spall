// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Pipeline/GraphicsPipeline/D3D12_GraphicsPipeline.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	GraphicsPipeline::GraphicsPipeline(
		Device& device,
		std::unique_ptr<RootSignature> rootSignature,
		ComPtr<ID3D12PipelineState> pipelineState,
		D3D12_PRIMITIVE_TOPOLOGY primitiveTopology,
		std::vector<VertexBindingInfo> vertexBindings,
		std::uint8_t stencilReference)
		: m_Device(&device),
		  m_RootSignature(std::move(rootSignature)),
		  m_PipelineState(std::move(pipelineState)),
		  m_PrimitiveTopology(primitiveTopology),
		  m_VertexBindings(std::move(vertexBindings)),
		  m_StencilReference(stencilReference)
	{
	}

	GraphicsPipeline::~GraphicsPipeline() = default;

	RenderBackendType GraphicsPipeline::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	PipelineType GraphicsPipeline::type() const
	{
		return PipelineType::Graphics;
	}
} // namespace spall::d3d12
