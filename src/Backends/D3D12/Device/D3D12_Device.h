#pragma once

#include <spall/Common/Resource/SharedObject.h>

#include <spall/Device/IDevice.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IPresentationFactory.h>
#include <spall/Device/IResourceFactory.h>
#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorHeap.h>
#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorRingPool.h>
#include <src/Backends/D3D12/Common/Resources/D3D12_ResourcePool.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Backends/D3D12/Pipeline/Mipmaps/D3D12_MipmapGenerator.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace spall::d3d12
{
	class ComputeQueue;
	class GraphicsQueue;

	class Device final : public SharedObject<IDevice>, public IResourceFactory, public IPipelineFactory, public IPresentationFactory
	{
	public:
		Device(
			ComPtr<IDXGIFactory4> factory,
			ComPtr<ID3D12Device> device);

		~Device(void) override;

		Status initialize(void);

		RenderBackendType backendType(void) const override;
		const DeviceLimits& limits(void) const override;

		Status queryFormatCapabilities(
			Format format,
			FormatCapabilities* capabilities) const override;

		IGraphicsQueue& graphicsQueue(void) override;
		IQueue& computeQueue(void) override;
		IResourceFactory& resources(void) override;
		IPipelineFactory& pipelines(void) override;
		IPresentationFactory& presentation(void) override;

		Status createCommandList(
			QueueType type,
			Resource<ICommandList>* commandList) override;

		Status createTexture(
			const TextureCreateInfo& info,
			Resource<ITexture>* texture) override;

		Status createTextureView(
			const TextureViewCreateInfo& info,
			Resource<ITextureView>* textureView) override;

		Status createFramebuffer(
			const FramebufferCreateInfo& createInfo,
			Resource<IFramebuffer>* framebuffer) override;

		Status createBuffer(
			const BufferCreateInfo& info,
			Resource<IBuffer>* buffer) override;

		Status createBufferWithData(
			const BufferCreateInfo& info,
			std::span<const std::byte> data,
			Resource<IBuffer>* buffer) override;

		Status writeBuffer(
			IBuffer& buffer,
			std::span<const std::byte> data,
			std::uint32_t offset) override;

		Status readBuffer(
			IBuffer& buffer,
			std::span<std::byte> data,
			std::uint32_t offset) override;

		Status createSampler(
			const SamplerCreateInfo& info,
			Resource<ISampler>* sampler) override;

		Status createQueryPool(
			const QueryPoolCreateInfo& info,
			Resource<IQueryPool>* queryPool) override;

		Status readTimestamps(
			IQueryPool& queryPool,
			std::uint32_t firstQuery,
			std::span<std::uint64_t> nanoseconds) override;

		Status createAccelerationStructure(
			const AccelerationStructureCreateInfo& info,
			Resource<IAccelerationStructure>* accelerationStructure) override;

		Status createSwapChain(
			const SwapChainCreateInfo& info,
			Resource<ISwapChain>* swapChain) override;

		Status createShader(
			const ShaderCreateInfo& info,
			Resource<IShader>* shader) override;

		Status createResourceSetLayout(
			const ResourceSetLayoutCreateInfo& info,
			Resource<IResourceSetLayout>* resourceSetLayout) override;

		Status createResourceSet(
			const ResourceSetCreateInfo& info,
			Resource<IResourceSet>* resourceSet) override;

		Status createPipeline(
			const PipelineCreateInfo& info,
			Resource<IPipeline>* pipeline) override;

		Status createComputePipeline(
			const ComputePipelineCreateInfo& info,
			Resource<IPipeline>* pipeline) override;

		Status createRayTracingPipeline(
			const RayTracingPipelineCreateInfo& info,
			Resource<IPipeline>* pipeline) override;

	private:
		/// Copies a whole buffer on a throwaway command list and waits for it, bypassing recorded state tracking.
		Status copyBufferImmediate(
			ID3D12Resource& destination,
			D3D12_RESOURCE_STATES destinationState,
			ID3D12Resource& source,
			std::uint64_t size);

		Status createIndirectCommandSignatures(void);

		ComPtr<IDXGIFactory4> m_Factory;
		ComPtr<ID3D12Device> m_Device;

		/// Held only when the adapter supports inline ray tracing, so a null
		/// interface and a cleared SupportsInlineRayTracing agree.
		ComPtr<ID3D12Device5> m_RayTracingDevice;

		ComPtr<ID3D12CommandQueue> m_CommandQueue;
		ComPtr<ID3D12CommandQueue> m_ComputeCommandQueue;

		ComPtr<ID3D12CommandAllocator> m_UploadCommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> m_UploadCommandList;

		ComPtr<ID3D12CommandSignature> m_DrawSignature;
		ComPtr<ID3D12CommandSignature> m_DrawIndexedSignature;
		ComPtr<ID3D12CommandSignature> m_DispatchSignature;

		DescriptorHeap m_RenderTargetViews;
		DescriptorHeap m_DepthStencilViews;
		DescriptorHeap m_ShaderResourceDescriptors;
		DescriptorHeap m_SamplerDescriptors;
		DescriptorRingPool m_DescriptorRingPool;
		ResourcePool m_ResourcePool;
		MipmapGenerator m_MipmapGenerator;
		std::uint64_t m_TimestampFrequency = 0;
		DeviceLimits m_Limits = {};

		std::unique_ptr<GraphicsQueue> m_GraphicsQueue;
		std::unique_ptr<ComputeQueue> m_ComputeQueue;

	private:
		friend class AccelerationStructure;
		friend class Buffer;
		friend class CommandList;
		friend class ComputePipeline;
		friend class Framebuffer;
		friend class GraphicsPipeline;
		friend class MipmapGenerator;
		friend class ComputeQueue;
		friend class GraphicsQueue;
		friend class QueryPool;
		friend class ResourcePool;
		friend class ResourceSet;
		friend class ResourceSetLayout;
		friend class RootSignature;
		friend class Sampler;
		friend class Shader;
		friend class SwapChain;
		friend class Texture;
		friend class TextureView;
	};
} // namespace spall::d3d12
