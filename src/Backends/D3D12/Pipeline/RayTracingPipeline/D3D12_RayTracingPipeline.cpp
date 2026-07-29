#include <src/Backends/D3D12/Pipeline/RayTracingPipeline/D3D12_RayTracingPipeline.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	RayTracingPipeline::RayTracingPipeline(
		Device& device,
		std::unique_ptr<RootSignature> rootSignature,
		ComPtr<ID3D12StateObject> stateObject,
		ComPtr<ID3D12Resource> shaderTable,
		const D3D12_DISPATCH_RAYS_DESC& dispatchDescription)
		: m_Device(&device), m_RootSignature(std::move(rootSignature)), m_StateObject(std::move(stateObject)), m_ShaderTable(std::move(shaderTable)), m_DispatchDescription(dispatchDescription)
	{
	}

	RayTracingPipeline::~RayTracingPipeline() = default;

	RenderBackendType RayTracingPipeline::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	PipelineType RayTracingPipeline::type() const
	{
		return PipelineType::RayTracing;
	}
} // namespace spall::d3d12
