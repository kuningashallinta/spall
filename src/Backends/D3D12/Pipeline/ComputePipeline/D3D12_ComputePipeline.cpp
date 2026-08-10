// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Pipeline/ComputePipeline/D3D12_ComputePipeline.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	ComputePipeline::ComputePipeline(
		Device& device,
		std::unique_ptr<RootSignature> rootSignature,
		ComPtr<ID3D12PipelineState> pipelineState)
		: m_Device(&device), m_RootSignature(std::move(rootSignature)), m_PipelineState(std::move(pipelineState))
	{
	}

	ComputePipeline::~ComputePipeline() = default;

	RenderBackendType ComputePipeline::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	PipelineType ComputePipeline::type() const
	{
		return PipelineType::Compute;
	}
} // namespace spall::d3d12
