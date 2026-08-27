// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/QueueType.h>
#include <spall/Common/Enums/ResourceStateFlags.h>
#include <src/Backends/Vulkan/CommandList/VK_RenderPassKey.h>
#include <src/Backends/Vulkan/CommandList/VK_ResourceStateTracker.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <vk_mem_alloc.h>

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace spall::vk
{
	class Device;
	class SwapChain;
	class Buffer;
	class GraphicsPipeline;
	class ComputePipeline;
	class RayTracingPipeline;
	class Sampler;
	class Texture;
	class QueryPool;
	class TextureView;
	class ResourceSet;
	class ResourceSetLayout;

	class CommandList final : public SharedObject<ICommandList>
	{
	public:
		CommandList(Device& device, QueueType queueType);
		~CommandList(void) override;

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

		Status compactAccelerationStructure(IAccelerationStructure& accelerationStructure) override;

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

		Status copyTexture(
			ITexture& destination,
			ITexture& source) override;

		Status generateMips(ITexture& texture) override;

		Status writeTimestamp(
			IQueryPool& queryPool,
			std::uint32_t query) override;

	private:
		// Reuse follows Initial/Completed -> Recording -> Executable -> Pending -> Completed.
		// Pending recordings cannot be reset or release their transient state.
		enum class ExecutionState
		{
			Initial,
			Recording,
			Executable,
			Pending,
			Completed,
			Invalid
		};

		struct VertexBufferBinding
		{
			Buffer* Resource = nullptr;
			std::uint32_t Stride = 0;
		};

		struct CachedRenderPass
		{
			RenderPassKey Key;
			VkRenderPass RenderPass = VK_NULL_HANDLE;
		};

		Status initialize(void);
		Status fail(Status error);

		Status validateDrawState(bool indexed) const;

		Status validateResourceSetAttachments(
			const ResourceSet& resourceSet,
			Texture* const* colorTextures,
			std::uint32_t colorTextureCount,
			Texture* depthTexture) const;

		Status requireBufferState(
			Buffer& buffer,
			ResourceStateFlags state);

		Status requireTextureState(
			Texture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources = {});

		Status requireTextureViewState(
			TextureView& textureView,
			ResourceStateFlags state);

		Status requireGraphicsResourceStates(bool indexed);

		Status bindComputeResourceSets(void);

		Status bindResourceSets(
			std::span<const Resource<ResourceSetLayout>> layouts,
			VkPipelineLayout pipelineLayout,
			VkPipelineBindPoint bindPoint);
		Status prepareDrawState(bool indexed);

		Status recordIndirectDraw(
			IBuffer& argumentBuffer,
			std::uint32_t offset,
			bool indexed);

		Status cachedRenderPass(
			const ColorAttachmentInfo* colorAttachments,
			std::uint32_t colorAttachmentCount,
			const DepthStencilAttachmentInfo* depthAttachment,
			const Format* colorFormats,
			Format depthFormat,
			std::uint32_t sampleCount,
			VkRenderPass& renderPass);

		Status beginNativeRenderPass(void);
		Status prepareGraphicsState(void);
		void endNativeRenderPass(void);
		bool isRenderPassAttachment(const Texture& texture) const;

		Status referencePresentTexture(Texture* texture);
		Status waitForSubmission(std::uint64_t submissionSerial);

		Status recordQueryResets(
			VkCommandBuffer commandBuffer,
			bool* recordedResets) const;

		void retainResource(IResource& resource);
		Status retainResourceSetDependencies(ResourceSet& resourceSet);
		void releaseResourceSetReferences(void);
		void resetTransientState(void);

		Resource<Device> m_Device;
		QueueType m_QueueType = QueueType::Graphics;

		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		VkCommandBuffer m_EntryCommandBuffer = VK_NULL_HANDLE;
		VkFence m_SubmissionFence = VK_NULL_HANDLE;

		ResourceStateTracker m_StateTracker;
		std::vector<std::pair<QueryPool*, std::uint32_t>> m_TimestampWrites;

		std::uint64_t m_SubmissionSerial = 0;
		std::uint64_t m_CompletedSubmissionSerial = 0;

		GraphicsPipeline* m_GraphicsPipeline = nullptr;
		ComputePipeline* m_ComputePipeline = nullptr;
		RayTracingPipeline* m_RayTracingPipeline = nullptr;
		std::vector<VertexBufferBinding> m_VertexBuffers;
		Buffer* m_IndexBuffer = nullptr;
		ResourceSet* m_BoundResourceSets[MaxResourceSets] = {};

		Resource<Texture> m_ReferencedPresentTexture;
		SwapChain* m_ReferencedSwapChain = nullptr;

		Texture* m_RenderPassColorTextures[MaxColorAttachments] = {};
		std::uint32_t m_RenderPassColorTextureCount = 0;
		Texture* m_RenderPassDepthTexture = nullptr;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkRenderPass m_ResumeRenderPass = VK_NULL_HANDLE;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
		std::vector<CachedRenderPass> m_RenderPassCache;
		std::vector<VkClearValue> m_RenderPassClearValues;
		std::uint32_t m_RenderPassSampleCount = 1;
		std::uint32_t m_RenderPassWidth = 0;
		std::uint32_t m_RenderPassHeight = 0;

		std::vector<Resource<IResource>> m_RetainedResources;

		struct RetiredAccelerationStructure
		{
			VkAccelerationStructureKHR AccelerationStructure = VK_NULL_HANDLE;
			VkBuffer Buffer = VK_NULL_HANDLE;
			VmaAllocation Allocation = VK_NULL_HANDLE;
		};

		std::vector<RetiredAccelerationStructure> m_RetiredAccelerationStructures;
		std::vector<Resource<ResourceSet>> m_LockedResourceSets;

		ExecutionState m_ExecutionState = ExecutionState::Initial;
		std::uint32_t m_DebugGroupDepth = 0;
		bool m_AutomaticBarriers = true;
		bool m_RenderPassActive = false;
		bool m_NativeRenderPassActive = false;
		bool m_RenderPassStarted = false;
		bool m_ViewportSet = false;
		bool m_ScissorSet = false;

	private:
		friend class Device;
		friend class GraphicsQueue;
	};
} // namespace spall::vk
