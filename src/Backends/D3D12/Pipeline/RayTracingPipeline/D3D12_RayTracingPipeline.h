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

	class RayTracingPipeline final : public SharedObject<IPipeline>
	{
	public:
		RayTracingPipeline(
			Device& device,
			std::unique_ptr<RootSignature> rootSignature,
			ComPtr<ID3D12StateObject> stateObject,
			ComPtr<ID3D12Resource> shaderTable,
			const D3D12_DISPATCH_RAYS_DESC& dispatchDescription);

		~RayTracingPipeline(void) override;

		RenderBackendType backendType(void) const override;
		PipelineType type(void) const override;

	private:
		Resource<Device> m_Device;

		std::unique_ptr<RootSignature> m_RootSignature;
		ComPtr<ID3D12StateObject> m_StateObject;

		/// Identifier-only shader binding table on the upload heap, whose
		/// generic-read state never changes.
		ComPtr<ID3D12Resource> m_ShaderTable;

		/// Prefilled table regions; a dispatch supplies only the grid size.
		D3D12_DISPATCH_RAYS_DESC m_DispatchDescription = {};

	private:
		friend class CommandList;
		friend class Device;
	};
} // namespace spall::d3d12
