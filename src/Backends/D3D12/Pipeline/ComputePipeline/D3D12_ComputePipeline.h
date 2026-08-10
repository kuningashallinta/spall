// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Backends/D3D12/Pipeline/RootSignature/D3D12_RootSignature.h>

#include <memory>

namespace spall::d3d12
{
	class CommandList;
	class Device;

	class ComputePipeline final : public SharedObject<IPipeline>
	{
	public:
		ComputePipeline(
			Device& device,
			std::unique_ptr<RootSignature> rootSignature,
			ComPtr<ID3D12PipelineState> pipelineState);

		~ComputePipeline(void) override;

		RenderBackendType backendType(void) const override;
		PipelineType type(void) const override;

	private:
		Resource<Device> m_Device;

		std::unique_ptr<RootSignature> m_RootSignature;
		ComPtr<ID3D12PipelineState> m_PipelineState;

	private:
		friend class CommandList;
		friend class Device;
	};
} // namespace spall::d3d12
