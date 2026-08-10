// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Enums/PipelineType.h>
#include <spall/Common/Enums/QueueType.h>
#include <src/Backends/D3D12/CommandList/D3D12_ResourceStateTracker.h>
#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorRingPool.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace spall::d3d12
{
	class AccelerationStructure;
	class Buffer;
	class ComputePipeline;
	class ComputeQueue;
	class Device;
	class FenceTimeline;
	class GraphicsPipeline;
	class GraphicsQueue;
	class QueryPool;
	class RayTracingPipeline;
	class ResourceSet;
	class RootSignature;
	class SwapChain;
	class Texture;
	class TextureView;

	class CommandList final : public SharedObject<ICommandList>
	{
	public:
		CommandList(Device& device, QueueType type);
		~CommandList(void) override;

		Status initialize(void);

		RenderBackendType backendType(void) const override;

		Status begin(void) override;
		Status end(void) override;

		Status pushDebugGroup(
			const char* label,
			Color color) override;

		Status popDebugGroup(void) override;

		Status insertDebugMarker(
			const char* label,
			Color color) override;

		Status beginRenderPass(const RenderPassBeginInfo& beginInfo) override;
		Status endRenderPass(void) override;

		Status setViewport(const Viewport& viewport) override;
		Status setScissor(const Scissor& scissor) override;
		Status setStencilReference(std::uint8_t reference) override;

		Status setVertexBuffer(
			std::uint32_t slot,
			IBuffer& buffer,
			std::uint32_t stride,
			std::uint32_t offset) override;

		Status setIndexBuffer(
			IBuffer& buffer,
			IndexFormat format,
			std::uint32_t offset) override;

		Status bindGraphicsPipeline(IPipeline& pipeline) override;
		Status bindComputePipeline(IPipeline& pipeline) override;
		Status bindRayTracingPipeline(IPipeline& pipeline) override;

		Status bindResourceSet(
			std::uint32_t slot,
			IResourceSet& resourceSet) override;

		Status setPushConstants(
			ShaderStageFlags stages,
			std::uint32_t offset,
			std::span<const std::byte> data) override;

		Status setEnableAutomaticBarriers(bool enable) override;

		Status beginTrackingBufferState(
			IBuffer& buffer,
			ResourceStateFlags state) override;

		Status beginTrackingTextureState(
			ITexture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources) override;

		Status setBufferState(
			IBuffer& buffer,
			ResourceStateFlags state) override;

		Status setTextureState(
			ITexture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources) override;

		Status setPermanentBufferState(
			IBuffer& buffer,
			ResourceStateFlags state) override;

		Status setPermanentTextureState(
			ITexture& texture,
			ResourceStateFlags state) override;

		Status commitBarriers(void) override;
		ResourceStateFlags bufferState(IBuffer& buffer) const override;

		ResourceStateFlags textureState(
			ITexture& texture,
			const TextureSubresourceRange& subresources) const override;

		Status draw(
			std::uint32_t vertexCount,
			std::uint32_t startVertex,
			std::uint32_t instanceCount,
			std::uint32_t startInstance) override;

		Status drawIndexed(
			std::uint32_t indexCount,
			std::uint32_t startIndex,
			std::int32_t vertexOffset,
			std::uint32_t instanceCount,
			std::uint32_t startInstance) override;

		Status dispatch(
			std::uint32_t groupCountX,
			std::uint32_t groupCountY,
			std::uint32_t groupCountZ) override;

		Status dispatchRays(
			std::uint32_t width,
			std::uint32_t height,
			std::uint32_t depth) override;

		Status drawIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset) override;

		Status drawIndexedIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset) override;

		Status dispatchIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset) override;

		Status compactAccelerationStructure(
			IAccelerationStructure& accelerationStructure) override;

		Status buildAccelerationStructure(
			IAccelerationStructure& accelerationStructure,
			const AccelerationStructureBuildInfo& buildInfo) override;

		Status copyBuffer(
			IBuffer& destination,
			std::uint32_t destinationOffset,
			IBuffer& source,
			std::uint32_t sourceOffset,
			std::uint32_t size) override;

		Status copyBufferToTexture(
			ITexture& destination,
			const TextureRegion& region,
			IBuffer& source,
			std::uint32_t sourceOffset,
			std::uint32_t sourceRowPitch) override;

		Status copyTextureToBuffer(
			IBuffer& destination,
			std::uint32_t destinationOffset,
			std::uint32_t destinationRowPitch,
			ITexture& source,
			const TextureRegion& region) override;

		Status generateMips(ITexture& texture) override;

		Status copyTexture(
			ITexture& destination,
			ITexture& source) override;

		Status writeTimestamp(
			IQueryPool& queryPool,
			std::uint32_t query) override;

	private:
		enum class ExecutionState
		{
			Initial,
			Recording,
			Executable,
			Pending,
			Completed,
			Invalid
		};

		Status fail(Status error);

		Status requireTextureState(
			Texture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources = {});

		Status requireBufferState(
			Buffer& buffer,
			ResourceStateFlags state);

		Status requireTextureViewState(
			TextureView& textureView,
			ResourceStateFlags state);

		Status referencePresentTexture(Texture* texture);
		void retainResource(IResource& resource);
		void resetTransientState(void);
		void reclaimAfterCompletion(void);

		/// Allocates a device-local scratch buffer retained until this recording completes.
		Status createScratchBuffer(
			std::uint64_t size,
			D3D12_RESOURCE_FLAGS flags,
			ID3D12Resource** resource);

		void transitionScratchBuffer(
			ID3D12Resource& resource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter);

		void transitionScratchSubresource(
			ID3D12Resource& resource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			UINT subresource);

		/// Allocates a render-targetable mirror of a texture, retained until this recording completes.
		Status createMipmapScratchTexture(
			const TextureInfo& info,
			ComPtr<ID3D12Resource>* resource);

		void releaseResourceSetReferences(void);
		void releaseScratchResources(void);
		void resolveTimestampWrites(void);

		Status requireResourceSetStates(const RootSignature& rootSignature);

		Status applyResourceSets(
			const RootSignature& rootSignature,
			PipelineType type);

		void applyPushConstants(
			const RootSignature& rootSignature,
			PipelineType type);

		Status prepareDraw(bool indexed);
		Status prepareDispatch(void);
		Status prepareRayDispatch(void);

		enum class IndirectKind
		{
			Draw,
			DrawIndexed,
			Dispatch
		};

		Status recordIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset,
			std::uint32_t argumentSize,
			ID3D12CommandSignature& commandSignature,
			IndirectKind kind);

		Resource<Device> m_Device;

		ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> m_CommandList;

		/// Available on any modern runtime, so support is decided by the device.
		ComPtr<ID3D12GraphicsCommandList4> m_RayTracingCommandList;

		D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ResourceStateTracker m_StateTracker;

		DescriptorRingSet m_Rings;

		GraphicsPipeline* m_GraphicsPipeline = nullptr;
		ComputePipeline* m_ComputePipeline = nullptr;
		RayTracingPipeline* m_RayTracingPipeline = nullptr;

		Resource<ResourceSet> m_BoundResourceSets[MaxResourceSets];

		std::vector<std::byte> m_PushConstantData;

		ExecutionState m_ExecutionState = ExecutionState::Initial;
		std::uint64_t m_SubmissionFenceValue = 0;
		FenceTimeline* m_SubmissionFence = nullptr;
		GraphicsQueue* m_ReclaimQueue = nullptr;

		std::vector<Resource<IResource>> m_RetainedResources;
		std::vector<std::pair<Resource<QueryPool>, std::uint32_t>> m_TimestampWrites;
		std::vector<ComPtr<ID3D12Resource>> m_RetainedScratchBuffers;

		/// Pre-compaction structures, released once the submission that copied
		/// out of them has completed.
		std::vector<ComPtr<ID3D12Resource>> m_RetiredAccelerationStructures;

		Resource<Texture> m_ReferencedPresentTexture;
		SwapChain* m_ReferencedSwapChain = nullptr;

		Texture* m_RenderPassColorTextures[MaxColorAttachments] = {};
		Texture* m_RenderPassResolveTextures[MaxColorAttachments] = {};
		std::uint32_t m_RenderPassColorTextureCount = 0;
		Texture* m_RenderPassDepthTexture = nullptr;

		std::uint32_t m_DebugGroupDepth = 0;

		bool m_AutomaticBarriers = true;
		bool m_RenderPassActive = false;
		bool m_ViewportSet = false;
		bool m_ScissorSet = false;
		bool m_IndexBufferSet = false;

	private:
		friend class ComputeQueue;
		friend class Device;
		friend class GraphicsQueue;
	};
} // namespace spall::d3d12
