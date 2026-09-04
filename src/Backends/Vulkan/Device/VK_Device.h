// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Device/IDevice.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IPresentationFactory.h>
#include <spall/Device/IResourceFactory.h>
#include <spall/RenderPass/ColorAttachmentInfo.h>
#include <spall/RenderPass/DepthStencilAttachmentInfo.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <vk_mem_alloc.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace spall::vk
{
	class GraphicsQueue;
	class SwapChain;
	class Texture;
	class TextureView;
	class Buffer;
	class Sampler;
	class Shader;
	class GraphicsPipeline;
	class ComputePipeline;
	class CommandList;
	class ResourceSetLayout;
	class SurfaceLifetime;
	class SwapChainGeneration;
	class AccelerationStructure;

	class Device final : public SharedObject<IDevice>, public IResourceFactory, public IPipelineFactory, public IPresentationFactory
	{
	public:
		Device(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			bool debugUtilsEnabled,
			VkPhysicalDevice physicalDevice,
			VkDevice device,
			std::uint32_t graphicsQueueFamilyIndex,
			VkQueue graphicsQueue,
			VkQueue computeQueue,
			VkCommandPool commandPool,
			bool rayTracingEnabled,
			bool rayTracingPipelineEnabled);

		~Device(void) override;

		RenderBackendType backendType(void) const override;
		const DeviceLimits& limits(void) const override;

		Status queryFormatCapabilities(
			Format format,
			FormatCapabilities* capabilities) const override;

		IGraphicsQueue& graphicsQueue(void) override;
		IQueue& computeQueue(void) override;
		Status createCommandList(
			QueueType type,
			Resource<ICommandList>* commandList) override;
		IResourceFactory& resources(void) override;
		IPipelineFactory& pipelines(void) override;
		IPresentationFactory& presentation(void) override;

		Status createTexture1D(
			const Texture1DCreateInfo& info,
			Resource<ITexture1D>* texture) override;

		Status createTexture2D(
			const Texture2DCreateInfo& info,
			Resource<ITexture2D>* texture) override;

		Status createTexture3D(
			const Texture3DCreateInfo& info,
			Resource<ITexture3D>* texture) override;

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

		VkCommandBuffer allocateCommandBuffer(void);
		void freeCommandBuffer(VkCommandBuffer commandBuffer);

		VkRenderPass renderPass(
			const Format* colorFormats,
			std::uint32_t colorFormatCount,
			Format depthFormat,
			std::uint32_t sampleCount);

	private:
		static VkImageAspectFlags imageAspectMask(Format format);

		Status createImage(
			const TextureInfo& info,
			VkImage* image,
			VmaAllocation* allocation);

		Status createPipelineLayout(
			std::span<const IResourceSetLayout* const> layouts,
			const PushConstantInfo& pushConstants,
			std::vector<Resource<ResourceSetLayout>>* resourceSetLayouts,
			VkPipelineLayout* pipelineLayout);

		Status buildRenderPass(
			const Format* colorFormats,
			std::uint32_t colorAttachmentCount,
			Format depthFormat,
			std::uint32_t sampleCount,
			const ColorAttachmentInfo* colorAttachments,
			const DepthStencilAttachmentInfo* depthAttachment,
			VkRenderPass& renderPass) const;

		Status createTransientRenderPass(
			const ColorAttachmentInfo* colorAttachments,
			std::uint32_t colorAttachmentCount,
			const DepthStencilAttachmentInfo* depthAttachment,
			const Format* colorFormats,
			Format depthFormat,
			std::uint32_t sampleCount,
			VkRenderPass& renderPass) const;

		Status createTransientFramebuffer(
			VkRenderPass renderPass,
			const VkImageView* attachments,
			std::uint32_t attachmentCount,
			std::uint32_t width,
			std::uint32_t height,
			VkFramebuffer& framebuffer) const;

		struct RenderPassCacheEntry
		{
			Format ColorFormats[MaxColorAttachments] = {};
			std::uint32_t ColorFormatCount = 0;
			Format DepthFormat = Format::Unknown;
			std::uint32_t SampleCount = 1;
			VkRenderPass RenderPass = VK_NULL_HANDLE;
		};

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_Properties = {};
		VkPhysicalDeviceMemoryProperties m_MemoryProperties = {};
		VkPhysicalDeviceFeatures m_SupportedFeatures = {};
		DeviceLimits m_Limits = {};

		VkDevice m_Device = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		PFN_vkCmdBeginDebugUtilsLabelEXT m_CmdBeginDebugUtilsLabel = nullptr;
		PFN_vkCmdEndDebugUtilsLabelEXT m_CmdEndDebugUtilsLabel = nullptr;
		PFN_vkCmdInsertDebugUtilsLabelEXT m_CmdInsertDebugUtilsLabel = nullptr;

		bool m_RayTracingEnabled = false;
		std::uint32_t m_ScratchOffsetAlignment = 0;
		PFN_vkCreateAccelerationStructureKHR m_CreateAccelerationStructure = nullptr;
		PFN_vkDestroyAccelerationStructureKHR m_DestroyAccelerationStructure = nullptr;
		PFN_vkGetAccelerationStructureBuildSizesKHR m_GetAccelerationStructureBuildSizes = nullptr;
		PFN_vkGetAccelerationStructureDeviceAddressKHR m_GetAccelerationStructureDeviceAddress = nullptr;
		PFN_vkCmdBuildAccelerationStructuresKHR m_CmdBuildAccelerationStructures = nullptr;
		PFN_vkCmdWriteAccelerationStructuresPropertiesKHR m_CmdWriteAccelerationStructuresProperties = nullptr;
		PFN_vkCmdCopyAccelerationStructureKHR m_CmdCopyAccelerationStructure = nullptr;

		bool m_RayTracingPipelineEnabled = false;
		std::uint32_t m_ShaderGroupHandleSize = 0;
		std::uint32_t m_ShaderGroupBaseAlignment = 0;
		std::uint32_t m_ShaderGroupHandleAlignment = 0;
		std::uint32_t m_MaxRayRecursionDepth = 0;
		PFN_vkCreateRayTracingPipelinesKHR m_CreateRayTracingPipelines = nullptr;
		PFN_vkGetRayTracingShaderGroupHandlesKHR m_GetRayTracingShaderGroupHandles = nullptr;
		PFN_vkCmdTraceRaysKHR m_CmdTraceRays = nullptr;
		PFN_vkGetBufferDeviceAddressKHR m_GetBufferDeviceAddress = nullptr;

		std::uint32_t m_GraphicsQueueFamilyIndex = 0;
		std::uint32_t m_TimestampValidBits = 0;
		float m_TimestampPeriod = 0.0f;
		std::unique_ptr<GraphicsQueue> m_GraphicsQueue;
		std::unique_ptr<GraphicsQueue> m_ComputeQueue;

		std::vector<RenderPassCacheEntry> m_RenderPassCache;

	private:
		friend class Texture;
		friend class Framebuffer;
		friend class TextureView;
		friend class Buffer;
		friend class Sampler;
		friend class QueryPool;
		friend class Shader;
		friend class GraphicsPipeline;
		friend class ComputePipeline;
		friend class RayTracingPipeline;
		friend class ResourceSet;
		friend class ResourceSetLayout;
		friend class CommandList;
		friend class GraphicsQueue;
		friend class SwapChain;
		friend class SurfaceLifetime;
		friend class SwapChainGeneration;
		friend class AccelerationStructure;
	};
} // namespace spall::vk
