#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/VertexInput/VertexBindingInfo.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Backends/D3D12/Pipeline/RootSignature/D3D12_RootSignature.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace spall::d3d12
{
	class CommandList;
	class Device;

	class GraphicsPipeline final : public SharedObject<IPipeline>
	{
	public:
		GraphicsPipeline(
			Device& device,
			std::unique_ptr<RootSignature> rootSignature,
			ComPtr<ID3D12PipelineState> pipelineState,
			D3D12_PRIMITIVE_TOPOLOGY primitiveTopology,
			std::vector<VertexBindingInfo> vertexBindings,
			std::uint8_t stencilReference);

		~GraphicsPipeline(void) override;

		RenderBackendType backendType(void) const override;
		PipelineType type(void) const override;

	private:
		Resource<Device> m_Device;

		std::unique_ptr<RootSignature> m_RootSignature;
		ComPtr<ID3D12PipelineState> m_PipelineState;

		D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		std::vector<VertexBindingInfo> m_VertexBindings;
		std::uint8_t m_StencilReference = 0;

	private:
		friend class CommandList;
		friend class Device;
	};
} // namespace spall::d3d12
