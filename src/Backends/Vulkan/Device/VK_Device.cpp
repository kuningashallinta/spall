// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <src/Backends/Vulkan/CommandList/VK_CommandList.h>
#include <src/Backends/Vulkan/Common/VK_FormatMappings.h>
#include <src/Backends/Vulkan/Queue/VK_GraphicsQueue.h>
#include <src/Validation/Common.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <algorithm>
#include <cstdint>
#include <memory>

namespace spall::vk
{
	Device::Device(
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
		bool rayTracingPipelineEnabled)
		: m_Instance(instance), m_DebugMessenger(debugMessenger), m_PhysicalDevice(physicalDevice), m_Device(device), m_GraphicsQueueFamilyIndex(graphicsQueueFamilyIndex), m_CommandPool(commandPool), m_RayTracingEnabled(rayTracingEnabled), m_RayTracingPipelineEnabled(rayTracingPipelineEnabled)
	{
		if (debugUtilsEnabled)
		{
			m_CmdBeginDebugUtilsLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdBeginDebugUtilsLabelEXT"));
			m_CmdEndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdEndDebugUtilsLabelEXT"));
			m_CmdInsertDebugUtilsLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_Device, "vkCmdInsertDebugUtilsLabelEXT"));
		}

		if (m_RayTracingEnabled)
		{
			m_CreateAccelerationStructure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_Device, "vkCreateAccelerationStructureKHR"));
			m_DestroyAccelerationStructure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_Device, "vkDestroyAccelerationStructureKHR"));
			m_GetAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_Device, "vkGetAccelerationStructureBuildSizesKHR"));
			m_GetAccelerationStructureDeviceAddress = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_Device, "vkGetAccelerationStructureDeviceAddressKHR"));
			m_CmdBuildAccelerationStructures = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdBuildAccelerationStructuresKHR"));
			m_CmdWriteAccelerationStructuresProperties = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
			m_CmdCopyAccelerationStructure = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdCopyAccelerationStructureKHR"));
			m_GetBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_Device, "vkGetBufferDeviceAddress"));

			m_RayTracingEnabled = (m_CreateAccelerationStructure != nullptr) and
				(m_DestroyAccelerationStructure != nullptr) and
				(m_GetAccelerationStructureBuildSizes != nullptr) and
				(m_GetAccelerationStructureDeviceAddress != nullptr) and
				(m_CmdBuildAccelerationStructures != nullptr) and
				(m_CmdWriteAccelerationStructuresProperties != nullptr) and
				(m_CmdCopyAccelerationStructure != nullptr) and
				(m_GetBufferDeviceAddress != nullptr);
		}

		m_RayTracingPipelineEnabled = m_RayTracingPipelineEnabled and m_RayTracingEnabled;

		if (m_RayTracingPipelineEnabled)
		{
			m_CreateRayTracingPipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_Device, "vkCreateRayTracingPipelinesKHR"));
			m_GetRayTracingShaderGroupHandles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_Device, "vkGetRayTracingShaderGroupHandlesKHR"));
			m_CmdTraceRays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdTraceRaysKHR"));

			m_RayTracingPipelineEnabled = (m_CreateRayTracingPipelines != nullptr) and
				(m_GetRayTracingShaderGroupHandles != nullptr) and
				(m_CmdTraceRays != nullptr);
		}

		if (m_RayTracingEnabled)
		{
			VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = {};
			rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

			VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties = {};
			accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
			accelerationStructureProperties.pNext = m_RayTracingPipelineEnabled ? &rayTracingPipelineProperties : nullptr;

			VkPhysicalDeviceProperties2 properties2 = {};
			properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			properties2.pNext = &accelerationStructureProperties;

			vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

			m_ScratchOffsetAlignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;

			if (m_RayTracingPipelineEnabled)
			{
				m_ShaderGroupHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
				m_ShaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
				m_ShaderGroupHandleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
				m_MaxRayRecursionDepth = rayTracingPipelineProperties.maxRayRecursionDepth;
			}
		}

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_SupportedFeatures);

		const VkPhysicalDeviceLimits& limits = m_Properties.limits;
		m_Limits.MaxTexture2DDimension = limits.maxImageDimension2D;
		m_Limits.MaxFramebufferWidth = limits.maxFramebufferWidth;
		m_Limits.MaxFramebufferHeight = limits.maxFramebufferHeight;
		m_Limits.MaxUniformBufferSize = limits.maxUniformBufferRange;
		m_Limits.MaxVertexBuffers = limits.maxVertexInputBindings;
		m_Limits.MaxVertexAttributes = limits.maxVertexInputAttributes;
		m_Limits.MaxVertexBufferStride = limits.maxVertexInputBindingStride;
		m_Limits.MaxUniformBuffersPerStage = limits.maxPerStageDescriptorUniformBuffers;
		m_Limits.MaxSampledTexturesPerStage = (std::min)(limits.maxPerStageDescriptorSampledImages, limits.maxPerStageDescriptorSamplers);
		m_Limits.MaxComputeStorageBuffers = limits.maxPerStageDescriptorStorageBuffers;
		m_Limits.MaxComputeStorageTextures = limits.maxPerStageDescriptorStorageImages;
		m_Limits.MaxColorAttachments = (std::min)(limits.maxColorAttachments, MaxColorAttachments);
		m_Limits.MaxResourceSets = (std::min)(limits.maxBoundDescriptorSets, MaxResourceSets);
		m_Limits.MaxPushConstantSize = (std::min)(limits.maxPushConstantsSize, MaxPushConstantSize);
		m_Limits.MaxComputeWorkGroupCount[0] = limits.maxComputeWorkGroupCount[0];
		m_Limits.MaxComputeWorkGroupCount[1] = limits.maxComputeWorkGroupCount[1];
		m_Limits.MaxComputeWorkGroupCount[2] = limits.maxComputeWorkGroupCount[2];
		m_Limits.MaxComputeWorkGroupSize[0] = limits.maxComputeWorkGroupSize[0];
		m_Limits.MaxComputeWorkGroupSize[1] = limits.maxComputeWorkGroupSize[1];
		m_Limits.MaxComputeWorkGroupSize[2] = limits.maxComputeWorkGroupSize[2];
		m_Limits.MaxComputeWorkGroupInvocations = limits.maxComputeWorkGroupInvocations;
		m_Limits.MaxSamplerAnisotropy = m_SupportedFeatures.samplerAnisotropy ? limits.maxSamplerAnisotropy : 1.0f;

		std::uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

		if (m_GraphicsQueueFamilyIndex < queueFamilyCount)
		{
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());
			m_TimestampValidBits = queueFamilies[m_GraphicsQueueFamilyIndex].timestampValidBits;
		}

		const VkSampleCountFlags framebufferSampleCounts = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
		m_Limits.SupportedSampleCounts = static_cast<std::uint32_t>(framebufferSampleCounts) & 0x7fu;

		m_TimestampPeriod = limits.timestampPeriod;
		m_Limits.SupportsTimestampQueries = (m_TimestampValidBits != 0) and (m_TimestampPeriod > 0.0f);
		m_Limits.SupportsInlineRayTracing = m_RayTracingEnabled;
		m_Limits.SupportsRayTracingPipeline = m_RayTracingPipelineEnabled;
		m_Limits.MaxRayRecursionDepth = m_MaxRayRecursionDepth;

		if (m_SupportedFeatures.wideLines)
		{
			m_Limits.MinLineWidth = limits.lineWidthRange[0];
			m_Limits.MaxLineWidth = limits.lineWidthRange[1];
		}

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.instance = m_Instance;
		allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
		allocatorCreateInfo.device = m_Device;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;

		if (m_RayTracingEnabled)
		{
			allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}

		vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);

		m_GraphicsQueue = std::make_unique<GraphicsQueue>(*this, graphicsQueue, graphicsQueueFamilyIndex);
		m_GraphicsQueue->initialize();
		m_ComputeQueue = std::make_unique<GraphicsQueue>(*this, computeQueue, graphicsQueueFamilyIndex);
		m_ComputeQueue->initialize();
	}

	Device::~Device()
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			const VkResult vkResult = vkDeviceWaitIdle(m_Device);

			SPALL_ASSERT((vkResult == VK_SUCCESS) or (vkResult == VK_ERROR_DEVICE_LOST));
		}

		m_GraphicsQueue.reset();
		m_ComputeQueue.reset();

		for (RenderPassCacheEntry& entry : m_RenderPassCache)
		{
			if ((m_Device != VK_NULL_HANDLE) and (entry.RenderPass != VK_NULL_HANDLE))
			{
				vkDestroyRenderPass(m_Device, entry.RenderPass, nullptr);
			}
		}

		if ((m_Device != VK_NULL_HANDLE) and (m_CommandPool != VK_NULL_HANDLE))
		{
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		}

		if (m_Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(m_Allocator);
		}

		if (m_Device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_Device, nullptr);
		}

		if ((m_Instance != VK_NULL_HANDLE) and (m_DebugMessenger != VK_NULL_HANDLE))
		{
			const PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(
					m_Instance,
					"vkDestroyDebugUtilsMessengerEXT"));

			if (destroyDebugMessenger != nullptr)
			{
				destroyDebugMessenger(m_Instance, m_DebugMessenger, nullptr);
			}
		}

		if (m_Instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_Instance, nullptr);
		}
	}

	RenderBackendType Device::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	const DeviceLimits& Device::limits() const
	{
		return m_Limits;
	}

	Status Device::queryFormatCapabilities(
		Format format,
		FormatCapabilities* capabilities) const
	{
		SPALL_TRY(validateFormatCapabilityQuery(format, capabilities));

		const std::optional<VkFormat> vkFormat = toVkFormat(format);

		if (not vkFormat.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		VkFormatProperties properties = {};
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, vkFormat.value(), &properties);
		*capabilities = formatCapabilities(format, properties);

		return {};
	}

	IGraphicsQueue& Device::graphicsQueue()
	{
		return *m_GraphicsQueue;
	}

	IQueue& Device::computeQueue()
	{
		return *m_ComputeQueue;
	}

	Status Device::createCommandList(
		QueueType type,
		Resource<ICommandList>* commandList)
	{
		if (commandList == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		std::unique_ptr<CommandList> vkCommandList = std::make_unique<CommandList>(*this, type);

		SPALL_TRY(vkCommandList->initialize());

		*commandList = Resource<ICommandList>(vkCommandList.release());

		return {};
	}

	IResourceFactory& Device::resources()
	{
		return *this;
	}

	IPipelineFactory& Device::pipelines()
	{
		return *this;
	}

	IPresentationFactory& Device::presentation()
	{
		return *this;
	}

	VkCommandBuffer Device::allocateCommandBuffer()
	{
		if ((m_Device == VK_NULL_HANDLE) or (m_CommandPool == VK_NULL_HANDLE))
		{
			return VK_NULL_HANDLE;
		}

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		const VkResult vkResult = vkAllocateCommandBuffers(m_Device, &allocateInfo, &commandBuffer);

		if (vkResult != VK_SUCCESS)
		{
			return VK_NULL_HANDLE;
		}

		return commandBuffer;
	}

	void Device::freeCommandBuffer(
		VkCommandBuffer commandBuffer)
	{
		if ((m_Device == VK_NULL_HANDLE) or (m_CommandPool == VK_NULL_HANDLE) or (commandBuffer == VK_NULL_HANDLE))
		{
			return;
		}

		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
	}
} // namespace spall::vk
