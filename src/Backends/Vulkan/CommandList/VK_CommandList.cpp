#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/CommandList/VK_CommandList.h>

#include <src/Backends/Vulkan/Common/VK_BackendCast.h>
#include <src/Backends/Vulkan/Common/VK_DebugLabel.h>
#include <src/Backends/Vulkan/Common/VK_EnumMappings.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Framebuffer/VK_Framebuffer.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSet.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>
#include <src/Backends/Vulkan/Pipeline/ComputePipeline/VK_ComputePipeline.h>
#include <src/Backends/Vulkan/Pipeline/GraphicsPipeline/VK_GraphicsPipeline.h>
#include <src/Backends/Vulkan/Pipeline/RayTracingPipeline/VK_RayTracingPipeline.h>
#include <src/Backends/Vulkan/Resources/AccelerationStructure/VK_AccelerationStructure.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_Buffer.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_BufferState.h>
#include <src/Backends/Vulkan/Resources/Query/VK_QueryPool.h>
#include <src/Backends/Vulkan/Resources/Sampler/VK_Sampler.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>
#include <src/Validation/Common.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace spall::vk
{
	CommandList::CommandList(
		Device& device,
		QueueType queueType)
		: m_Device(&device), m_QueueType(queueType), m_StateTracker(VK_NULL_HANDLE),
		  m_VertexBuffers(device.m_Properties.limits.maxVertexInputBindings)
	{
	}

	Status CommandList::initialize()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			m_ExecutionState = ExecutionState::Invalid;

			return ERR_INVALID_STATE;
		}

		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = m_Device->m_GraphicsQueueFamilyIndex;

		VkResult vkResult = vkCreateCommandPool(m_Device->m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool);

		if (vkResult != VK_SUCCESS)
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapVulkanStatus(vkResult);
		}

		VkCommandBuffer commandBuffers[2] = {};

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 2;

		vkResult = vkAllocateCommandBuffers(m_Device->m_Device, &allocateInfo, commandBuffers);

		if (vkResult != VK_SUCCESS)
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapVulkanStatus(vkResult);
		}

		m_EntryCommandBuffer = commandBuffers[0];
		m_CommandBuffer = commandBuffers[1];
		m_StateTracker = ResourceStateTracker(m_CommandBuffer);

		VkFenceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		vkResult = vkCreateFence(m_Device->m_Device, &createInfo, nullptr, &m_SubmissionFence);

		if (vkResult != VK_SUCCESS)
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapVulkanStatus(vkResult);
		}

		return {};
	}

	CommandList::~CommandList()
	{
		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE))
		{
			if ((m_SubmissionFence != VK_NULL_HANDLE) and (m_SubmissionSerial > m_CompletedSubmissionSerial))
			{
				vkWaitForFences(m_Device->m_Device, 1, &m_SubmissionFence, VK_TRUE, UINT64_MAX);
			}

			for (const RetiredAccelerationStructure& retired : m_RetiredAccelerationStructures)
			{
				m_Device->m_DestroyAccelerationStructure(m_Device->m_Device, retired.AccelerationStructure, nullptr);
				vmaDestroyBuffer(m_Device->m_Allocator, retired.Buffer, retired.Allocation);
			}

			m_RetiredAccelerationStructures.clear();

			for (const CachedRenderPass& cached : m_RenderPassCache)
			{
				if (cached.RenderPass != VK_NULL_HANDLE)
				{
					vkDestroyRenderPass(m_Device->m_Device, cached.RenderPass, nullptr);
				}
			}

			if (m_SubmissionFence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device->m_Device, m_SubmissionFence, nullptr);
			}

			if (m_CommandPool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(m_Device->m_Device, m_CommandPool, nullptr);
			}
		}

		releaseResourceSetReferences();
	}

	RenderBackendType CommandList::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	Status CommandList::fail(
		Status error)
	{
		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE) and (m_CommandBuffer != VK_NULL_HANDLE) and
			(m_ExecutionState == ExecutionState::Recording))
		{
			endNativeRenderPass();
			m_RenderPassActive = false;

			vkEndCommandBuffer(m_CommandBuffer);
		}

		if (m_ExecutionState != ExecutionState::Pending)
		{
			m_ExecutionState = ExecutionState::Invalid;
		}

		return error;
	}

	Status CommandList::validateDrawState(
		bool indexed) const
	{
		const std::span<const VertexBindingInfo> pipelineBindings = (m_GraphicsPipeline != nullptr)
			? std::span<const VertexBindingInfo>(m_GraphicsPipeline->m_VertexBindings)
			: std::span<const VertexBindingInfo>();

		return spall::validateDrawState(
			m_RenderPassActive,
			m_GraphicsPipeline != nullptr,
			m_ViewportSet,
			m_ScissorSet,
			pipelineBindings,
			std::span<const VertexBufferBinding>(m_VertexBuffers),
			m_IndexBuffer != nullptr,
			indexed);
	}

	Status CommandList::requireBufferState(
		Buffer& buffer,
		ResourceStateFlags state)
	{
		if (not m_AutomaticBarriers)
		{
			return {};
		}

		return m_StateTracker.requireBufferState(buffer, state);
	}

	Status CommandList::requireTextureState(
		Texture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		if (not m_AutomaticBarriers)
		{
			return {};
		}

		return m_StateTracker.requireTextureState(texture, state, subresources);
	}

	Status CommandList::requireTextureViewState(
		TextureView& textureView,
		ResourceStateFlags state)
	{
		return requireTextureState(
			*textureView.m_Texture,
			state,
			TextureSubresourceRange {
				textureView.m_BaseMipLevel,
				textureView.m_MipLevels,
				textureView.m_BaseArrayLayer,
				textureView.m_ArrayLayers});
	}

	Status CommandList::beginNativeRenderPass()
	{
		if (m_NativeRenderPassActive)
		{
			return {};
		}

		const VkRenderPass renderPass = m_RenderPassStarted ? m_ResumeRenderPass : m_RenderPass;
		const VkFramebuffer framebuffer = m_Framebuffer;

		if ((renderPass == VK_NULL_HANDLE) or (framebuffer == VK_NULL_HANDLE))
		{
			return ERR_INVALID_STATE;
		}

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffer;
		beginInfo.renderArea.offset.x = 0;
		beginInfo.renderArea.offset.y = 0;
		beginInfo.renderArea.extent.width = m_RenderPassWidth;
		beginInfo.renderArea.extent.height = m_RenderPassHeight;
		beginInfo.clearValueCount = static_cast<std::uint32_t>(m_RenderPassClearValues.size());
		beginInfo.pClearValues = m_RenderPassClearValues.data();

		vkCmdBeginRenderPass(m_CommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		m_NativeRenderPassActive = true;
		m_RenderPassStarted = true;

		return {};
	}

	void CommandList::endNativeRenderPass()
	{
		if (m_NativeRenderPassActive)
		{
			vkCmdEndRenderPass(m_CommandBuffer);

			VkMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

			vkCmdPipelineBarrier(
				m_CommandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0,
				1,
				&barrier,
				0,
				nullptr,
				0,
				nullptr);

			m_NativeRenderPassActive = false;
		}
	}

	Status CommandList::prepareGraphicsState()
	{
		if (m_StateTracker.hasPendingBarriers())
		{
			endNativeRenderPass();

			Status error = m_StateTracker.commitBarriers();

			if (error != SUCCESS)
			{
				return error;
			}
		}

		return beginNativeRenderPass();
	}

	bool CommandList::isRenderPassAttachment(
		const Texture& texture) const
	{
		for (std::uint32_t attachmentIndex = 0; attachmentIndex < m_RenderPassColorTextureCount; ++attachmentIndex)
		{
			if (m_RenderPassColorTextures[attachmentIndex] == &texture)
			{
				return true;
			}
		}

		return m_RenderPassDepthTexture == &texture;
	}

	Status CommandList::requireGraphicsResourceStates(
		bool indexed)
	{
		std::unordered_map<Buffer*, std::uint32_t> requiredStates;
		std::unordered_map<TextureView*, std::uint32_t> requiredTextureStates;

		for (const VertexBindingInfo& binding : m_GraphicsPipeline->m_VertexBindings)
		{
			Buffer* buffer = m_VertexBuffers[binding.Binding].Resource;
			requiredStates[buffer] |= static_cast<std::uint32_t>(ResourceStateFlags::VertexBuffer);
		}

		if (indexed)
		{
			requiredStates[m_IndexBuffer] |= static_cast<std::uint32_t>(ResourceStateFlags::IndexBuffer);
		}

		for (std::size_t slotIndex = 0; slotIndex < m_GraphicsPipeline->m_ResourceSetLayouts.size(); ++slotIndex)
		{
			ResourceSet* resourceSet = m_BoundResourceSets[slotIndex];
			ResourceSetLayout* layout = m_GraphicsPipeline->m_ResourceSetLayouts[slotIndex].get();

			if ((resourceSet == nullptr) or (layout == nullptr))
			{
				return ERR_INVALID_BINDING;
			}

			for (const ResourceBindingInfo& bindingInfo : layout->m_Bindings)
			{
				const ResourceWrite* write = resourceSet->findWrite(bindingInfo.Binding);

				if ((write != nullptr) and (write->Type == ResourceBindingType::UniformBuffer))
				{
					Buffer* buffer = backendCast<Buffer>(write->Buffer);

					if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					requiredStates[buffer] |= static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer);
				}
				else if ((write != nullptr) and (write->Type == ResourceBindingType::StorageBuffer))
				{
					Buffer* buffer = backendCast<Buffer>(write->Buffer);

					if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					requiredStates[buffer] |= static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess);
				}
				else if ((write != nullptr) and (write->Type == ResourceBindingType::StorageTexture))
				{
					TextureView* textureView = backendCast<TextureView>(write->TextureView);

					if ((textureView == nullptr) or (not textureView->m_Texture) or
						(textureView->m_Texture->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					if (isRenderPassAttachment(*textureView->m_Texture))
					{
						return ERR_INVALID_RESOURCE_STATE;
					}

					requiredTextureStates[textureView] |= static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess);
				}
				else if ((write != nullptr) and (write->Type == ResourceBindingType::AccelerationStructure))
				{
					AccelerationStructure* structure = backendCast<AccelerationStructure>(write->AccelerationStructure);

					if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					if (not structure->m_Built)
					{
						return ERR_INVALID_STATE;
					}
				}
			}
		}

		for (const auto& requiredState : requiredStates)
		{
			Status error = requireBufferState(*requiredState.first, static_cast<ResourceStateFlags>(requiredState.second));

			if (error != SUCCESS)
			{
				return error;
			}
		}

		for (const auto& requiredState : requiredTextureStates)
		{
			Status error = requireTextureViewState(*requiredState.first, static_cast<ResourceStateFlags>(requiredState.second));

			if (error != SUCCESS)
			{
				return error;
			}
		}

		return {};
	}

	void CommandList::retainResource(
		IResource& resource)
	{
		for (const Resource<IResource>& retainedResource : m_RetainedResources)
		{
			if (retainedResource.get() == &resource)
			{
				return;
			}
		}

		m_RetainedResources.emplace_back(&resource);
	}

	Status CommandList::retainResourceSetDependencies(
		ResourceSet& resourceSet)
	{
		retainResource(resourceSet);

		for (const ResourceWrite& write : resourceSet.m_Writes)
		{
			if (write.Type == ResourceBindingType::UniformBuffer)
			{
				Buffer* buffer = backendCast<Buffer>(write.Buffer);

				if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
				{
					return ERR_INVALID_RESOURCE;
				}

				retainResource(*buffer);
			}
			else if (write.Type == ResourceBindingType::SampledTexture)
			{
				TextureView* textureView = backendCast<TextureView>(write.TextureView);
				Sampler* sampler = backendCast<Sampler>(write.Sampler);

				if ((textureView == nullptr) or (not textureView->m_Texture) or
					(textureView->m_Texture->m_Device.get() != m_Device.get()) or (sampler == nullptr) or (sampler->m_Device.get() != m_Device.get()))
				{
					return ERR_INVALID_RESOURCE;
				}

				retainResource(*textureView);
				retainResource(*sampler);
			}
			else if (write.Type == ResourceBindingType::StorageBuffer)
			{
				Buffer* buffer = backendCast<Buffer>(write.Buffer);

				if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
				{
					return ERR_INVALID_RESOURCE;
				}

				retainResource(*buffer);
			}
			else if (write.Type == ResourceBindingType::StorageTexture)
			{
				TextureView* textureView = backendCast<TextureView>(write.TextureView);

				if ((textureView == nullptr) or (not textureView->m_Texture) or
					(textureView->m_Texture->m_Device.get() != m_Device.get()))
				{
					return ERR_INVALID_RESOURCE;
				}

				retainResource(*textureView);
			}
			else if (write.Type == ResourceBindingType::AccelerationStructure)
			{
				AccelerationStructure* structure = backendCast<AccelerationStructure>(write.AccelerationStructure);

				if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
				{
					return ERR_INVALID_RESOURCE;
				}

				retainResource(*structure);
			}
			else
			{
				return ERR_UNSUPPORTED_USAGE;
			}
		}

		return {};
	}

	Status CommandList::waitForSubmission(
		std::uint64_t submissionSerial)
	{
		if ((submissionSerial == 0) or (submissionSerial > m_SubmissionSerial))
		{
			return ERR_INVALID_STATE;
		}

		if (submissionSerial <= m_CompletedSubmissionSerial)
		{
			return {};
		}

		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE) or
			(m_SubmissionFence == VK_NULL_HANDLE) or (m_ExecutionState != ExecutionState::Pending) or
			(submissionSerial != m_SubmissionSerial))
		{
			return ERR_INVALID_STATE;
		}

		const VkResult vkResult = vkWaitForFences(m_Device->m_Device, 1, &m_SubmissionFence, VK_TRUE, UINT64_MAX);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		m_CompletedSubmissionSerial = submissionSerial;
		m_ExecutionState = ExecutionState::Completed;
		releaseResourceSetReferences();

		return {};
	}

	void CommandList::releaseResourceSetReferences()
	{
		for (const Resource<ResourceSet>& resourceSet : m_LockedResourceSets)
		{
			SPALL_ASSERT(resourceSet);
			SPALL_VERIFY(resourceSet->m_CommandListReferenceCount != 0);
			--resourceSet->m_CommandListReferenceCount;
		}

		m_LockedResourceSets.clear();
	}

	void CommandList::resetTransientState()
	{
		m_GraphicsPipeline = nullptr;
		m_ComputePipeline = nullptr;
		m_RayTracingPipeline = nullptr;
		m_IndexBuffer = nullptr;
		m_ViewportSet = false;
		m_ScissorSet = false;

		for (VertexBufferBinding& binding : m_VertexBuffers)
		{
			binding = {};
		}

		m_ReferencedPresentTexture.reset();
		m_ReferencedSwapChain = nullptr;

		for (std::uint32_t textureIndex = 0; textureIndex < MaxColorAttachments; ++textureIndex)
		{
			m_RenderPassColorTextures[textureIndex] = nullptr;
		}

		for (std::uint32_t slotIndex = 0; slotIndex < MaxResourceSets; ++slotIndex)
		{
			m_BoundResourceSets[slotIndex] = nullptr;
		}

		m_RenderPassColorTextureCount = 0;
		m_RenderPassDepthTexture = nullptr;
		m_RenderPass = VK_NULL_HANDLE;
		m_Framebuffer = VK_NULL_HANDLE;
		m_ResumeRenderPass = VK_NULL_HANDLE;
		m_RenderPassClearValues.clear();
		m_RenderPassWidth = 0;
		m_RenderPassHeight = 0;
		m_DebugGroupDepth = 0;
		m_RenderPassActive = false;
		m_NativeRenderPassActive = false;
		m_RenderPassStarted = false;

		m_StateTracker.reset();
		m_RetainedResources.clear();

		for (const RetiredAccelerationStructure& retired : m_RetiredAccelerationStructures)
		{
			m_Device->m_DestroyAccelerationStructure(m_Device->m_Device, retired.AccelerationStructure, nullptr);
			vmaDestroyBuffer(m_Device->m_Allocator, retired.Buffer, retired.Allocation);
		}

		m_RetiredAccelerationStructures.clear();
		releaseResourceSetReferences();
	}

	Status CommandList::referencePresentTexture(
		Texture* texture)
	{
		if ((texture != nullptr) and (texture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((texture == nullptr) or not texture->m_IsSwapChainTexture)
		{
			return {};
		}

		if (m_ReferencedPresentTexture and (m_ReferencedPresentTexture.get() != texture))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		retainResource(*texture);
		m_ReferencedPresentTexture.reset(texture);
		m_ReferencedSwapChain = texture->m_SwapChain;

		return {};
	}

	Status CommandList::validateResourceSetAttachments(
		const ResourceSet& resourceSet,
		Texture* const* colorTextures,
		std::uint32_t colorTextureCount,
		Texture* depthTexture) const
	{
		const void* attachmentTextures[MaxColorAttachments + 1] = {};
		std::uint32_t attachmentTextureCount = 0;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorTextureCount; ++attachmentIndex)
		{
			if (colorTextures[attachmentIndex] != nullptr)
			{
				attachmentTextures[attachmentTextureCount++] = colorTextures[attachmentIndex];
			}
		}

		if (depthTexture != nullptr)
		{
			attachmentTextures[attachmentTextureCount++] = depthTexture;
		}

		for (const ResourceWrite& write : resourceSet.m_Writes)
		{
			if (write.Type != ResourceBindingType::SampledTexture)
			{
				continue;
			}

			TextureView* sampledView = backendCast<TextureView>(write.TextureView);

			if ((sampledView == nullptr) or (not sampledView->m_Texture))
			{
				return ERR_INVALID_RESOURCE;
			}

			const void* sampledTexture = sampledView->m_Texture.get();

			SPALL_TRY(validateNoSampledAttachmentAliasing(
				std::span<const void* const>(&sampledTexture, 1),
				std::span<const void* const>(attachmentTextures, attachmentTextureCount)));
		}

		return {};
	}

	Status CommandList::begin()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE) or
			(m_CommandBuffer == VK_NULL_HANDLE) or (m_SubmissionFence == VK_NULL_HANDLE))
		{
			return ERR_INVALID_STATE;
		}

		if (m_ExecutionState == ExecutionState::Pending)
		{
			const VkResult fenceStatus = vkGetFenceStatus(m_Device->m_Device, m_SubmissionFence);

			if (fenceStatus == VK_NOT_READY)
			{
				return ERR_INVALID_STATE;
			}

			if (fenceStatus != VK_SUCCESS)
			{
				return fail(mapVulkanStatus(fenceStatus));
			}

			m_CompletedSubmissionSerial = m_SubmissionSerial;
			m_ExecutionState = ExecutionState::Completed;
		}

		if (m_ExecutionState == ExecutionState::Invalid)
		{
			m_ExecutionState = ExecutionState::Initial;
			m_RenderPassActive = false;
		}

		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, false);

		if (error != SUCCESS)
		{
			return error;
		}

		resetTransientState();
		m_TimestampWrites.clear();

		VkResult vkResult = vkResetCommandBuffer(m_CommandBuffer, 0);

		if (vkResult != VK_SUCCESS)
		{
			return fail(mapVulkanStatus(vkResult));
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkResult = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);

		if (vkResult != VK_SUCCESS)
		{
			return fail(mapVulkanStatus(vkResult));
		}

		m_ExecutionState = ExecutionState::Recording;

		return {};
	}

	Status CommandList::end()
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_DebugGroupDepth != 0)
		{
			return ERR_INVALID_STATE;
		}

		if (m_RenderPassActive)
		{
			error = endRenderPass();

			if (error != SUCCESS)
			{
				return error;
			}
		}

		if (m_ReferencedPresentTexture)
		{
			error = requireTextureState(*m_ReferencedPresentTexture, ResourceStateFlags::Present);

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		error = m_StateTracker.keepInitialStates();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		const VkResult vkResult = vkEndCommandBuffer(m_CommandBuffer);

		if (vkResult != VK_SUCCESS)
		{
			m_ExecutionState = ExecutionState::Invalid;

			return fail(mapVulkanStatus(vkResult));
		}

		m_ExecutionState = ExecutionState::Executable;

		return {};
	}

	Status CommandList::pushDebugGroup(
		const char* label,
		Color color)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));
		SPALL_TRY(validateDebugLabel(label));

		if (m_Device->m_CmdBeginDebugUtilsLabel != nullptr)
		{
			const VkDebugUtilsLabelEXT labelInfo = debugUtilsLabel(label, color);
			m_Device->m_CmdBeginDebugUtilsLabel(m_CommandBuffer, &labelInfo);
		}

		++m_DebugGroupDepth;

		return {};
	}

	Status CommandList::popDebugGroup()
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_DebugGroupDepth == 0)
		{
			return ERR_INVALID_STATE;
		}

		if (m_Device->m_CmdEndDebugUtilsLabel != nullptr)
		{
			m_Device->m_CmdEndDebugUtilsLabel(m_CommandBuffer);
		}

		--m_DebugGroupDepth;

		return {};
	}

	Status CommandList::insertDebugMarker(
		const char* label,
		Color color)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));
		SPALL_TRY(validateDebugLabel(label));

		if (m_Device->m_CmdInsertDebugUtilsLabel != nullptr)
		{
			const VkDebugUtilsLabelEXT labelInfo = debugUtilsLabel(label, color);
			m_Device->m_CmdInsertDebugUtilsLabel(m_CommandBuffer, &labelInfo);
		}

		return {};
	}

	Status CommandList::cachedRenderPass(
		const ColorAttachmentInfo* colorAttachments,
		std::uint32_t colorAttachmentCount,
		const DepthStencilAttachmentInfo* depthAttachment,
		const Format* colorFormats,
		Format depthFormat,
		std::uint32_t sampleCount,
		VkRenderPass& renderPass)
	{
		RenderPassKey key = {};
		key.ColorCount = colorAttachmentCount;
		key.SampleCount = sampleCount;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorAttachmentCount; ++attachmentIndex)
		{
			key.ColorFormats[attachmentIndex] = colorFormats[attachmentIndex];
			key.ColorLoadActions[attachmentIndex] = colorAttachments[attachmentIndex].LoadAction;
			key.ColorStoreActions[attachmentIndex] = colorAttachments[attachmentIndex].StoreAction;
		}

		if (depthAttachment != nullptr)
		{
			key.DepthFormat = depthFormat;
			key.DepthLoadAction = depthAttachment->DepthLoadAction;
			key.DepthStoreAction = depthAttachment->DepthStoreAction;
			key.StencilLoadAction = depthAttachment->StencilLoadAction;
			key.StencilStoreAction = depthAttachment->StencilStoreAction;
		}

		for (const CachedRenderPass& cached : m_RenderPassCache)
		{
			if (cached.Key == key)
			{
				renderPass = cached.RenderPass;
				return {};
			}
		}

		Status error = m_Device->createTransientRenderPass(
			colorAttachments,
			colorAttachmentCount,
			depthAttachment,
			colorFormats,
			depthFormat,
			sampleCount,
			renderPass);

		if (error != SUCCESS)
		{
			return error;
		}

		m_RenderPassCache.push_back(CachedRenderPass {key, renderPass});

		return {};
	}

	Status CommandList::beginRenderPass(
		const RenderPassBeginInfo& beginInfo)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		error = validatePassBeginInfo(beginInfo);

		if (error != SUCCESS)
		{
			return error;
		}

		Framebuffer* framebuffer = backendCast<Framebuffer>(beginInfo.Framebuffer);

		if (framebuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (framebuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		std::vector<TextureView*> colorViews;
		std::vector<Format> colorFormats;
		Texture* colorTextures[MaxColorAttachments] = {};

		colorViews.reserve(framebuffer->m_ColorCount);
		colorFormats.reserve(framebuffer->m_ColorCount);

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			TextureView* colorView = framebuffer->m_ColorViews[attachmentIndex].get();
			colorViews.push_back(colorView);
			colorFormats.push_back(colorView->m_Texture->m_Info.Format);
			colorTextures[attachmentIndex] = colorView->m_Texture.get();
		}

		TextureView* depthView = framebuffer->m_DepthView.get();
		Texture* depthTexture = depthView != nullptr ? depthView->m_Texture.get() : nullptr;

		for (ResourceSet* boundResourceSet : m_BoundResourceSets)
		{
			if (boundResourceSet == nullptr)
			{
				continue;
			}

			error = validateResourceSetAttachments(
				*boundResourceSet,
				colorTextures,
				framebuffer->m_ColorCount,
				depthTexture);

			if (error != SUCCESS)
			{
				return error;
			}
		}

		retainResource(*framebuffer);

		for (TextureView* colorView : colorViews)
		{
			error = referencePresentTexture(colorView->m_Texture.get());

			if (error != SUCCESS)
			{
				return fail(error);
			}

			error = requireTextureViewState(*colorView, ResourceStateFlags::RenderTarget);

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			TextureView* resolveView = framebuffer->m_ResolveViews[attachmentIndex].get();

			if (resolveView == nullptr)
			{
				continue;
			}

			error = referencePresentTexture(resolveView->m_Texture.get());

			if (error != SUCCESS)
			{
				return fail(error);
			}

			error = requireTextureViewState(*resolveView, ResourceStateFlags::RenderTarget);

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		if (depthView != nullptr)
		{
			error = requireTextureViewState(*depthView, ResourceStateFlags::DepthWrite);

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		std::vector<ColorAttachmentInfo> initialColorAttachments(framebuffer->m_ColorCount);
		std::vector<ColorAttachmentInfo> resumeColorAttachments(framebuffer->m_ColorCount);

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			initialColorAttachments[attachmentIndex] = beginInfo.ColorAttachments[attachmentIndex];
			initialColorAttachments[attachmentIndex].StoreAction = StoreAction::Store;
			resumeColorAttachments[attachmentIndex] = initialColorAttachments[attachmentIndex];
			resumeColorAttachments[attachmentIndex].LoadAction = LoadAction::Load;
		}

		DepthStencilAttachmentInfo initialDepthAttachment = beginInfo.DepthAttachment;
		initialDepthAttachment.DepthStoreAction = StoreAction::Store;
		initialDepthAttachment.StencilStoreAction = StoreAction::Store;

		DepthStencilAttachmentInfo resumeDepthAttachment = initialDepthAttachment;
		resumeDepthAttachment.DepthLoadAction = LoadAction::Load;
		resumeDepthAttachment.StencilLoadAction = LoadAction::Load;

		const Format depthFormat = depthView != nullptr ? depthView->m_Texture->m_Info.Format : Format::Unknown;

		error = cachedRenderPass(
			initialColorAttachments.data(),
			framebuffer->m_ColorCount,
			depthView != nullptr ? &initialDepthAttachment : nullptr,
			colorFormats.data(),
			depthFormat,
			framebuffer->m_Info.SampleCount,
			m_RenderPass);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = cachedRenderPass(
			resumeColorAttachments.data(),
			framebuffer->m_ColorCount,
			depthView != nullptr ? &resumeDepthAttachment : nullptr,
			colorFormats.data(),
			depthFormat,
			framebuffer->m_Info.SampleCount,
			m_ResumeRenderPass);

		if (error != SUCCESS)
		{
			m_RenderPass = VK_NULL_HANDLE;

			return fail(error);
		}

		m_Framebuffer = framebuffer->m_Framebuffer;

		m_RenderPassClearValues.resize(framebuffer->m_ColorCount + (depthView != nullptr ? 1u : 0u));

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			m_RenderPassClearValues[attachmentIndex].color.float32[0] = beginInfo.ColorAttachments[attachmentIndex].ClearColor.R;
			m_RenderPassClearValues[attachmentIndex].color.float32[1] = beginInfo.ColorAttachments[attachmentIndex].ClearColor.G;
			m_RenderPassClearValues[attachmentIndex].color.float32[2] = beginInfo.ColorAttachments[attachmentIndex].ClearColor.B;
			m_RenderPassClearValues[attachmentIndex].color.float32[3] = beginInfo.ColorAttachments[attachmentIndex].ClearColor.A;
		}

		if (depthView != nullptr)
		{
			m_RenderPassClearValues[framebuffer->m_ColorCount].depthStencil.depth = beginInfo.DepthAttachment.ClearDepth;
			m_RenderPassClearValues[framebuffer->m_ColorCount].depthStencil.stencil = beginInfo.DepthAttachment.ClearStencil;
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			m_RenderPassColorTextures[attachmentIndex] = colorViews[attachmentIndex]->m_Texture.get();
		}

		m_RenderPassColorTextureCount = framebuffer->m_ColorCount;
		m_RenderPassDepthTexture = depthView != nullptr ? depthView->m_Texture.get() : nullptr;
		m_RenderPassSampleCount = framebuffer->m_Info.SampleCount;
		m_RenderPassWidth = framebuffer->m_Info.Width;
		m_RenderPassHeight = framebuffer->m_Info.Height;
		m_RenderPassActive = true;
		m_NativeRenderPassActive = false;
		m_RenderPassStarted = false;

		return {};
	}

	Status CommandList::endRenderPass()
	{
		if (not m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not m_RenderPassStarted)
		{
			Status error = beginNativeRenderPass();

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		endNativeRenderPass();
		m_RenderPassActive = false;

		Status error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		m_Framebuffer = VK_NULL_HANDLE;
		m_RenderPass = VK_NULL_HANDLE;
		m_ResumeRenderPass = VK_NULL_HANDLE;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < m_RenderPassColorTextureCount; ++attachmentIndex)
		{
			m_RenderPassColorTextures[attachmentIndex] = nullptr;
		}

		m_RenderPassColorTextureCount = 0;
		m_RenderPassDepthTexture = nullptr;
		m_RenderPassClearValues.clear();
		m_RenderPassWidth = 0;
		m_RenderPassHeight = 0;
		m_RenderPassStarted = false;
		return {};
	}

	Status CommandList::setViewport(
		const Viewport& viewport)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		error = validateViewport(viewport);

		if (error != SUCCESS)
		{
			return error;
		}

		const VkPhysicalDeviceLimits& limits = m_Device->m_Properties.limits;
		const float right = viewport.X + viewport.Width;
		const float bottom = viewport.Y + viewport.Height;

		if ((viewport.Width > static_cast<float>(limits.maxViewportDimensions[0])) or
			(viewport.Height > static_cast<float>(limits.maxViewportDimensions[1])) or
			(viewport.X < limits.viewportBoundsRange[0]) or (right > limits.viewportBoundsRange[1]) or
			(viewport.Y < limits.viewportBoundsRange[0]) or (bottom > limits.viewportBoundsRange[1]))
		{
			return ERR_INVALID_RANGE;
		}

		VkViewport nativeViewport = {};
		nativeViewport.x = viewport.X;
		nativeViewport.y = viewport.Y;
		nativeViewport.width = viewport.Width;
		nativeViewport.height = viewport.Height;
		nativeViewport.minDepth = viewport.MinDepth;
		nativeViewport.maxDepth = viewport.MaxDepth;

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &nativeViewport);
		m_ViewportSet = true;

		return {};
	}

	Status CommandList::setScissor(
		const Scissor& scissor)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		error = validateScissor(scissor);

		if (error != SUCCESS)
		{
			return error;
		}

		VkRect2D nativeScissor = {};
		nativeScissor.offset.x = scissor.X;
		nativeScissor.offset.y = scissor.Y;
		nativeScissor.extent.width = scissor.Width;
		nativeScissor.extent.height = scissor.Height;

		vkCmdSetScissor(m_CommandBuffer, 0, 1, &nativeScissor);
		m_ScissorSet = true;

		return {};
	}

	Status CommandList::setStencilReference(
		std::uint8_t reference)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_GraphicsPipeline == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		vkCmdSetStencilReference(m_CommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, reference);

		return {};
	}

	Status CommandList::setVertexBuffer(
		std::uint32_t slot,
		IBuffer& buffer,
		std::uint32_t stride,
		std::uint32_t offset)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if ((backendBuffer == nullptr) or (backendBuffer->m_Device.get() != m_Device.get()) or (backendBuffer->m_Buffer == VK_NULL_HANDLE) or
			((backendBuffer->m_Info.Usage & BufferUsageFlags::Vertex) == BufferUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((slot >= m_VertexBuffers.size()) or (stride == 0) or (offset >= backendBuffer->m_Info.Size))
		{
			return ERR_INVALID_BINDING;
		}

		retainResource(*backendBuffer);

		const VkDeviceSize offsets[] = {offset};
		const VkBuffer buffers[] = {backendBuffer->m_Buffer};
		vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, buffers, offsets);
		m_VertexBuffers[slot] = VertexBufferBinding {backendBuffer, stride};

		return {};
	}

	Status CommandList::setIndexBuffer(
		IBuffer& buffer,
		IndexFormat format,
		std::uint32_t offset)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if ((backendBuffer == nullptr) or (backendBuffer->m_Device.get() != m_Device.get()) or (backendBuffer->m_Buffer == VK_NULL_HANDLE) or
			((backendBuffer->m_Info.Usage & BufferUsageFlags::Index) == BufferUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		const std::uint32_t indexSize = (format == IndexFormat::UInt16) ? 2u : 4u;

		if (((format != IndexFormat::UInt16) and (format != IndexFormat::UInt32)) or
			(offset >= backendBuffer->m_Info.Size) or ((offset % indexSize) != 0))
		{
			return ERR_INVALID_BINDING;
		}

		retainResource(*backendBuffer);

		vkCmdBindIndexBuffer(m_CommandBuffer, backendBuffer->m_Buffer, offset, vulkanIndexType(format));
		m_IndexBuffer = backendBuffer;

		return {};
	}

	Status CommandList::bindGraphicsPipeline(
		IPipeline& pipeline)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (pipeline.type() != PipelineType::Graphics)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		GraphicsPipeline* vulkanPipeline = backendCast<GraphicsPipeline>(pipeline);

		if ((vulkanPipeline == nullptr) or (vulkanPipeline->m_Device.get() != m_Device.get()) or (vulkanPipeline->m_Pipeline == VK_NULL_HANDLE))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (m_RenderPassColorTextureCount != vulkanPipeline->m_ColorTargetFormatCount)
		{
			return ERR_INVALID_FORMAT;
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < m_RenderPassColorTextureCount; ++attachmentIndex)
		{
			if ((m_RenderPassColorTextures[attachmentIndex] == nullptr) or
				(vulkanPipeline->m_ColorTargetFormats[attachmentIndex] != m_RenderPassColorTextures[attachmentIndex]->m_Info.Format))
			{
				return ERR_INVALID_FORMAT;
			}
		}

		if ((m_RenderPassDepthTexture != nullptr) and (vulkanPipeline->m_DepthStencilFormat != m_RenderPassDepthTexture->m_Info.Format))
		{
			return ERR_INVALID_FORMAT;
		}

		if ((m_RenderPassDepthTexture == nullptr) and (vulkanPipeline->m_DepthStencilFormat != Format::Unknown))
		{
			return ERR_INVALID_FORMAT;
		}

		if (vulkanPipeline->m_SampleCount != m_RenderPassSampleCount)
		{
			return ERR_INVALID_RESOURCE;
		}

		retainResource(*vulkanPipeline);

		if (m_GraphicsPipeline != vulkanPipeline)
		{
			m_GraphicsPipeline = vulkanPipeline;

			for (std::uint32_t slotIndex = 0; slotIndex < MaxResourceSets; ++slotIndex)
			{
				m_BoundResourceSets[slotIndex] = nullptr;
			}
		}

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->m_Pipeline);
		vkCmdSetStencilReference(
			m_CommandBuffer,
			VK_STENCIL_FACE_FRONT_AND_BACK,
			vulkanPipeline->m_StencilReference);

		return {};
	}

	Status CommandList::bindComputePipeline(
		IPipeline& pipeline)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (pipeline.type() != PipelineType::Compute)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		ComputePipeline* vulkanPipeline = backendCast<ComputePipeline>(pipeline);

		if ((vulkanPipeline == nullptr) or (vulkanPipeline->m_Device.get() != m_Device.get()) or (vulkanPipeline->m_Pipeline == VK_NULL_HANDLE))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		retainResource(*vulkanPipeline);

		if (m_ComputePipeline != vulkanPipeline)
		{
			m_ComputePipeline = vulkanPipeline;

			for (std::uint32_t slotIndex = 0; slotIndex < MaxResourceSets; ++slotIndex)
			{
				m_BoundResourceSets[slotIndex] = nullptr;
			}
		}

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipeline->m_Pipeline);

		return {};
	}

	Status CommandList::bindRayTracingPipeline(
		IPipeline& pipeline)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not m_Device->m_RayTracingPipelineEnabled)
		{
			return ERR_UNSUPPORTED;
		}

		if (m_QueueType != QueueType::Graphics)
		{
			return ERR_UNSUPPORTED;
		}

		if (pipeline.type() != PipelineType::RayTracing)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		RayTracingPipeline* vulkanPipeline = backendCast<RayTracingPipeline>(pipeline);

		if ((vulkanPipeline == nullptr) or (vulkanPipeline->m_Device.get() != m_Device.get()) or (vulkanPipeline->m_Pipeline == VK_NULL_HANDLE))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		retainResource(*vulkanPipeline);

		if (m_RayTracingPipeline != vulkanPipeline)
		{
			m_RayTracingPipeline = vulkanPipeline;

			for (std::uint32_t slotIndex = 0; slotIndex < MaxResourceSets; ++slotIndex)
			{
				m_BoundResourceSets[slotIndex] = nullptr;
			}
		}

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vulkanPipeline->m_Pipeline);

		return {};
	}

	Status CommandList::bindResourceSet(
		std::uint32_t slot,
		IResourceSet& resourceSet)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if ((not m_RenderPassActive) and (m_ComputePipeline == nullptr) and (m_RayTracingPipeline == nullptr))
		{
			return ERR_INVALID_STATE;
		}

		if (slot >= MaxResourceSets)
		{
			return ERR_INVALID_BINDING;
		}

		ResourceSet* backendResourceSet = backendCast<ResourceSet>(resourceSet);

		if ((backendResourceSet == nullptr) or (backendResourceSet->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateResourceSetAttachments(
			*backendResourceSet,
			m_RenderPassColorTextures,
			m_RenderPassColorTextureCount,
			m_RenderPassDepthTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		bool resourceSetLocked = false;

		for (const Resource<ResourceSet>& lockedResourceSet : m_LockedResourceSets)
		{
			if (lockedResourceSet.get() == backendResourceSet)
			{
				resourceSetLocked = true;
				break;
			}
		}

		if (not resourceSetLocked)
		{
			SPALL_ASSERT(backendResourceSet->m_CommandListReferenceCount < (std::numeric_limits<std::uint32_t>::max)());

			if (backendResourceSet->m_CommandListReferenceCount == (std::numeric_limits<std::uint32_t>::max)())
			{
				return ERR_INVALID_STATE;
			}

			m_LockedResourceSets.emplace_back(backendResourceSet);
			++backendResourceSet->m_CommandListReferenceCount;
		}

		retainResource(*backendResourceSet);
		m_BoundResourceSets[slot] = backendResourceSet;

		return {};
	}

	Status CommandList::setPushConstants(
		ShaderStageFlags stages,
		std::uint32_t offset,
		std::span<const std::byte> data)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		constexpr ShaderStageFlags rayTracingStages = ShaderStageFlags::RayGeneration | ShaderStageFlags::Miss |
			ShaderStageFlags::ClosestHit | ShaderStageFlags::AnyHit | ShaderStageFlags::Intersection;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		ShaderStageFlags declaredStages = ShaderStageFlags::None;
		std::uint32_t declaredSize = 0;

		if ((stages & rayTracingStages) != ShaderStageFlags::None)
		{
			if (m_RayTracingPipeline == nullptr)
			{
				return ERR_INVALID_STATE;
			}

			pipelineLayout = m_RayTracingPipeline->m_PipelineLayout;
			declaredStages = m_RayTracingPipeline->m_PushConstantStages;
			declaredSize = m_RayTracingPipeline->m_PushConstantSize;
		}
		else if (stages == ShaderStageFlags::Compute)
		{
			if (m_ComputePipeline == nullptr)
			{
				return ERR_INVALID_STATE;
			}

			pipelineLayout = m_ComputePipeline->m_PipelineLayout;
			declaredStages = m_ComputePipeline->m_PushConstantStages;
			declaredSize = m_ComputePipeline->m_PushConstantSize;
		}
		else
		{
			if (m_GraphicsPipeline == nullptr)
			{
				return ERR_INVALID_STATE;
			}

			pipelineLayout = m_GraphicsPipeline->m_PipelineLayout;
			declaredStages = m_GraphicsPipeline->m_PushConstantStages;
			declaredSize = m_GraphicsPipeline->m_PushConstantSize;
		}

		error = validatePushConstantUpdate(declaredStages, declaredSize, stages, offset, data.size());

		if (error != SUCCESS)
		{
			return error;
		}

		vkCmdPushConstants(
			m_CommandBuffer,
			pipelineLayout,
			vulkanShaderStageFlags(stages),
			offset,
			static_cast<std::uint32_t>(data.size()),
			data.data());

		return {};
	}

	Status CommandList::setEnableAutomaticBarriers(
		bool enable)
	{
		m_AutomaticBarriers = enable;
		return {};
	}

	Status CommandList::beginTrackingBufferState(
		IBuffer& buffer,
		ResourceStateFlags state)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* vulkanBuffer = backendCast<Buffer>(buffer);

		if ((vulkanBuffer == nullptr) or (vulkanBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateBufferResourceState(vulkanBuffer->m_Info, state);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanBuffer);
		error = m_StateTracker.beginTrackingBufferState(*vulkanBuffer, state);

		if (error != SUCCESS)
		{
			return error;
		}

		return {};
	}

	Status CommandList::beginTrackingTextureState(
		ITexture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Texture* vulkanTexture = backendCast<Texture>(texture);

		if ((vulkanTexture == nullptr) or (vulkanTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateTextureResourceState(vulkanTexture->m_Info, state, vulkanTexture->m_IsSwapChainTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanTexture);
		error = m_StateTracker.beginTrackingTextureState(*vulkanTexture, state, subresources);

		if (error != SUCCESS)
		{
			return error;
		}

		return {};
	}

	Status CommandList::setBufferState(
		IBuffer& buffer,
		ResourceStateFlags state)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* vulkanBuffer = backendCast<Buffer>(buffer);

		if ((vulkanBuffer == nullptr) or (vulkanBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateBufferResourceState(vulkanBuffer->m_Info, state);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanBuffer);
		error = m_StateTracker.requireBufferState(*vulkanBuffer, state);

		if (error != SUCCESS)
		{
			return error;
		}

		return {};
	}

	Status CommandList::setTextureState(
		ITexture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Texture* vulkanTexture = backendCast<Texture>(texture);

		if ((vulkanTexture == nullptr) or (vulkanTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateTextureResourceState(vulkanTexture->m_Info, state, vulkanTexture->m_IsSwapChainTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive and isRenderPassAttachment(*vulkanTexture))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		error = referencePresentTexture(vulkanTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanTexture);
		error = m_StateTracker.requireTextureState(*vulkanTexture, state, subresources);

		if (error != SUCCESS)
		{
			return error;
		}

		return {};
	}

	Status CommandList::setPermanentBufferState(
		IBuffer& buffer,
		ResourceStateFlags state)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* vulkanBuffer = backendCast<Buffer>(buffer);

		if ((vulkanBuffer == nullptr) or (vulkanBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateBufferResourceState(vulkanBuffer->m_Info, state);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanBuffer);
		error = m_StateTracker.setPermanentBufferState(*vulkanBuffer, state);

		if (error != SUCCESS)
		{
			return error;
		}

		return {};
	}

	Status CommandList::setPermanentTextureState(
		ITexture& texture,
		ResourceStateFlags state)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Texture* vulkanTexture = backendCast<Texture>(texture);

		if ((vulkanTexture == nullptr) or (vulkanTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateTextureResourceState(vulkanTexture->m_Info, state, vulkanTexture->m_IsSwapChainTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive and isRenderPassAttachment(*vulkanTexture))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		error = referencePresentTexture(vulkanTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanTexture);
		error = m_StateTracker.setPermanentTextureState(*vulkanTexture, state);

		if (error != SUCCESS)
		{
			return error;
		}

		return {};
	}

	Status CommandList::commitBarriers()
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_StateTracker.hasPendingBarriers())
		{
			endNativeRenderPass();
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		return {};
	}

	ResourceStateFlags CommandList::bufferState(
		IBuffer& buffer) const
	{
		Buffer* vulkanBuffer = backendCast<Buffer>(buffer);

		return ((vulkanBuffer != nullptr) and (vulkanBuffer->m_Device.get() == m_Device.get()))
			? m_StateTracker.currentBufferState(*vulkanBuffer)
			: ResourceStateFlags::Unknown;
	}

	ResourceStateFlags CommandList::textureState(
		ITexture& texture,
		const TextureSubresourceRange& subresources) const
	{
		Texture* vulkanTexture = backendCast<Texture>(texture);

		return ((vulkanTexture != nullptr) and (vulkanTexture->m_Device.get() == m_Device.get()))
			? m_StateTracker.currentTextureState(*vulkanTexture, subresources)
			: ResourceStateFlags::Unknown;
	}

	Status CommandList::prepareDrawState(
		bool indexed)
	{
		Status error;

		error = validateDrawState(indexed);

		if (error != SUCCESS)
		{
			return error;
		}

		error = requireGraphicsResourceStates(indexed);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		std::vector<VkDescriptorSet> descriptorSets;
		descriptorSets.reserve(m_GraphicsPipeline->m_ResourceSetLayouts.size());

		for (std::size_t slotIndex = 0; slotIndex < m_GraphicsPipeline->m_ResourceSetLayouts.size(); ++slotIndex)
		{
			ResourceSet* resourceSet = m_BoundResourceSets[slotIndex];
			ResourceSetLayout* layout = m_GraphicsPipeline->m_ResourceSetLayouts[slotIndex].get();

			if ((resourceSet == nullptr) or (layout == nullptr))
			{
				return fail(ERR_INVALID_BINDING);
			}

			if (&resourceSet->layout() != layout)
			{
				return fail(ERR_INVALID_BINDING);
			}

			for (const ResourceBindingInfo& bindingInfo : layout->m_Bindings)
			{
				const ResourceWrite* write = resourceSet->findWrite(bindingInfo.Binding);

				if (write == nullptr)
				{
					return fail(ERR_INVALID_BINDING);
				}

				if (write->Type == ResourceBindingType::SampledTexture)
				{
					if (write->TextureView == nullptr)
					{
						return fail(ERR_INVALID_RESOURCE);
					}

					TextureView* textureView = backendCast<TextureView>(write->TextureView);

					if ((textureView == nullptr) or (not textureView->m_Texture))
					{
						return fail(ERR_INVALID_RESOURCE_TYPE);
					}

					if (isRenderPassAttachment(*textureView->m_Texture))
					{
						return fail(ERR_INVALID_RESOURCE_STATE);
					}

					error = referencePresentTexture(textureView->m_Texture.get());

					if (error != SUCCESS)
					{
						return fail(error);
					}

					error = requireTextureViewState(*textureView, ResourceStateFlags::ShaderResource);

					if (error != SUCCESS)
					{
						return fail(error);
					}
				}
			}

			error = retainResourceSetDependencies(*resourceSet);

			if (error != SUCCESS)
			{
				return fail(error);
			}

			descriptorSets.push_back(resourceSet->m_DescriptorSet);
		}

		error = prepareGraphicsState();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		if (not descriptorSets.empty())
		{
			vkCmdBindDescriptorSets(
				m_CommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_GraphicsPipeline->m_PipelineLayout,
				0,
				static_cast<std::uint32_t>(descriptorSets.size()),
				descriptorSets.data(),
				0,
				nullptr);
		}

		return {};
	}

	Status CommandList::draw(
		std::uint32_t vertexCount,
		std::uint32_t startVertex,
		std::uint32_t instanceCount,
		std::uint32_t startInstance)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		error = prepareDrawState(false);

		if (error != SUCCESS)
		{
			return error;
		}

		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, startVertex, startInstance);

		return {};
	}

	Status CommandList::drawIndexed(
		std::uint32_t indexCount,
		std::uint32_t startIndex,
		std::int32_t vertexOffset,
		std::uint32_t instanceCount,
		std::uint32_t startInstance)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		error = prepareDrawState(true);

		if (error != SUCCESS)
		{
			return error;
		}

		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, startIndex, vertexOffset, startInstance);

		return {};
	}

	Status CommandList::drawIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset)
	{
		return recordIndirectDraw(argumentBuffer, offset, false);
	}

	Status CommandList::drawIndexedIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset)
	{
		return recordIndirectDraw(argumentBuffer, offset, true);
	}

	Status CommandList::recordIndirectDraw(
		IBuffer& argumentBuffer,
		std::uint32_t offset,
		bool indexed)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* vulkanBuffer = backendCast<Buffer>(argumentBuffer);

		if ((vulkanBuffer == nullptr) or (vulkanBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		const std::uint32_t argumentSize = indexed
			? static_cast<std::uint32_t>(sizeof(DrawIndexedIndirectCommand))
			: static_cast<std::uint32_t>(sizeof(DrawIndirectCommand));

		error = validateIndirectArguments(argumentBuffer, offset, argumentSize);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanBuffer);
		error = requireBufferState(*vulkanBuffer, ResourceStateFlags::IndirectArgument);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = prepareDrawState(indexed);

		if (error != SUCCESS)
		{
			return error;
		}

		if (indexed)
		{
			vkCmdDrawIndexedIndirect(m_CommandBuffer, vulkanBuffer->m_Buffer, offset, 1, 0);
		}
		else
		{
			vkCmdDrawIndirect(m_CommandBuffer, vulkanBuffer->m_Buffer, offset, 1, 0);
		}

		return {};
	}

	Status CommandList::bindComputeResourceSets()
	{
		return bindResourceSets(m_ComputePipeline->m_ResourceSetLayouts, m_ComputePipeline->m_PipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE);
	}

	Status CommandList::bindResourceSets(
		std::span<const Resource<ResourceSetLayout>> layouts,
		VkPipelineLayout pipelineLayout,
		VkPipelineBindPoint bindPoint)
	{
		std::vector<VkDescriptorSet> descriptorSets;
		std::unordered_map<Buffer*, std::uint32_t> requiredBufferStates;
		std::unordered_map<TextureView*, std::uint32_t> requiredTextureStates;
		descriptorSets.reserve(layouts.size());

		for (std::size_t slotIndex = 0; slotIndex < layouts.size(); ++slotIndex)
		{
			ResourceSet* resourceSet = m_BoundResourceSets[slotIndex];
			ResourceSetLayout* layout = layouts[slotIndex].get();

			if ((resourceSet == nullptr) or (layout == nullptr))
			{
				return ERR_INVALID_BINDING;
			}

			if (&resourceSet->layout() != layout)
			{
				return ERR_INVALID_BINDING;
			}

			for (const ResourceBindingInfo& bindingInfo : layout->m_Bindings)
			{
				const ResourceWrite* write = resourceSet->findWrite(bindingInfo.Binding);

				if (write == nullptr)
				{
					return ERR_INVALID_BINDING;
				}

				if (write->Type == ResourceBindingType::UniformBuffer)
				{
					Buffer* buffer = backendCast<Buffer>(write->Buffer);

					if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					requiredBufferStates[buffer] |= static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer);
				}
				else if (write->Type == ResourceBindingType::StorageBuffer)
				{
					Buffer* buffer = backendCast<Buffer>(write->Buffer);

					if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					requiredBufferStates[buffer] |= static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess);
				}
				else if (write->Type == ResourceBindingType::SampledTexture)
				{
					TextureView* textureView = backendCast<TextureView>(write->TextureView);

					if ((textureView == nullptr) or (not textureView->m_Texture))
					{
						return ERR_INVALID_RESOURCE_TYPE;
					}

					Status reference = referencePresentTexture(textureView->m_Texture.get());

					if (reference != SUCCESS)
					{
						return reference;
					}

					requiredTextureStates[textureView] |= static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource);
				}
				else if (write->Type == ResourceBindingType::StorageTexture)
				{
					TextureView* textureView = backendCast<TextureView>(write->TextureView);

					if ((textureView == nullptr) or (not textureView->m_Texture))
					{
						return ERR_INVALID_RESOURCE_TYPE;
					}

					requiredTextureStates[textureView] |= static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess);
				}
				else if (write->Type == ResourceBindingType::AccelerationStructure)
				{
					AccelerationStructure* structure = backendCast<AccelerationStructure>(write->AccelerationStructure);

					if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE;
					}

					if (not structure->m_Built)
					{
						return ERR_INVALID_STATE;
					}
				}
			}

			Status retain = retainResourceSetDependencies(*resourceSet);

			if (retain != SUCCESS)
			{
				return retain;
			}

			descriptorSets.push_back(resourceSet->m_DescriptorSet);
		}

		for (const auto& requiredState : requiredBufferStates)
		{
			Status transition = requireBufferState(*requiredState.first, static_cast<ResourceStateFlags>(requiredState.second));

			if (transition != SUCCESS)
			{
				return transition;
			}
		}

		for (const auto& requiredState : requiredTextureStates)
		{
			Status transition = requireTextureViewState(*requiredState.first, static_cast<ResourceStateFlags>(requiredState.second));

			if (transition != SUCCESS)
			{
				return transition;
			}
		}

		if (not descriptorSets.empty())
		{
			vkCmdBindDescriptorSets(
				m_CommandBuffer,
				bindPoint,
				pipelineLayout,
				0,
				static_cast<std::uint32_t>(descriptorSets.size()),
				descriptorSets.data(),
				0,
				nullptr);
		}

		return {};
	}

	Status CommandList::dispatch(
		std::uint32_t groupCountX,
		std::uint32_t groupCountY,
		std::uint32_t groupCountZ)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		error = spall::validateDispatchState(m_RenderPassActive, m_ComputePipeline != nullptr);

		if (error != SUCCESS)
		{
			return error;
		}

		error = bindComputeResourceSets();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		vkCmdDispatch(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);

		return {};
	}

	Status CommandList::dispatchRays(
		std::uint32_t width,
		std::uint32_t height,
		std::uint32_t depth)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not m_Device->m_RayTracingPipelineEnabled)
		{
			return ERR_UNSUPPORTED;
		}

		if (m_QueueType != QueueType::Graphics)
		{
			return ERR_UNSUPPORTED;
		}

		if (m_RayTracingPipeline == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		error = bindResourceSets(m_RayTracingPipeline->m_ResourceSetLayouts, m_RayTracingPipeline->m_PipelineLayout, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		const VkStridedDeviceAddressRegionKHR callableRegion = {};

		m_Device->m_CmdTraceRays(
			m_CommandBuffer,
			&m_RayTracingPipeline->m_RayGenerationRegion,
			&m_RayTracingPipeline->m_MissRegion,
			&m_RayTracingPipeline->m_HitRegion,
			&callableRegion,
			width,
			height,
			depth);

		return {};
	}

	Status CommandList::dispatchIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		error = spall::validateDispatchState(m_RenderPassActive, m_ComputePipeline != nullptr);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* vulkanBuffer = backendCast<Buffer>(argumentBuffer);

		if ((vulkanBuffer == nullptr) or (vulkanBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateIndirectArguments(argumentBuffer, offset, static_cast<std::uint32_t>(sizeof(DispatchIndirectCommand)));

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*vulkanBuffer);
		error = requireBufferState(*vulkanBuffer, ResourceStateFlags::IndirectArgument);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = bindComputeResourceSets();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		vkCmdDispatchIndirect(m_CommandBuffer, vulkanBuffer->m_Buffer, offset);

		return {};
	}

	Status CommandList::buildAccelerationStructure(
		IAccelerationStructure& accelerationStructure,
		const AccelerationStructureBuildInfo& buildInfo)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not m_Device->m_RayTracingEnabled)
		{
			return ERR_UNSUPPORTED;
		}

		if (m_QueueType != QueueType::Graphics)
		{
			return ERR_UNSUPPORTED;
		}

		AccelerationStructure* structure = backendCast<AccelerationStructure>(accelerationStructure);

		if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		SPALL_TRY(validateAccelerationStructureBuildInfo(structure->m_Info, buildInfo));

		if (buildInfo.Update and (not structure->m_Built))
		{
			return ERR_INVALID_STATE;
		}

		if (structure->m_Compacted)
		{
			return ERR_INVALID_STATE;
		}

		const bool topLevel = (structure->m_Info.Type == AccelerationStructureType::TopLevel);

		const std::uint32_t instanceCount = (buildInfo.InstanceCount != 0)
			? buildInfo.InstanceCount
			: structure->m_Info.InstanceCount;

		if (topLevel and buildInfo.Update and (instanceCount != structure->m_BuiltInstanceCount))
		{
			return ERR_INVALID_RANGE;
		}

		for (const Resource<Buffer>& inputBuffer : structure->m_InputBuffers)
		{
			Status stateError = requireBufferState(*inputBuffer, ResourceStateFlags::ShaderResource);

			if (stateError != SUCCESS)
			{
				return fail(stateError);
			}
		}

		Status barrierError = m_StateTracker.commitBarriers();

		if (barrierError != SUCCESS)
		{
			return fail(barrierError);
		}

		VkMemoryBarrier buildBarrier = {};
		buildBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		buildBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		buildBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0,
			1,
			&buildBarrier,
			0,
			nullptr,
			0,
			nullptr);

		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {};
		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.type = vulkanAccelerationStructureType(structure->m_Info.Type);
		buildGeometryInfo.flags = vulkanAccelerationStructureBuildFlags(structure->m_Info.Flags);
		buildGeometryInfo.srcAccelerationStructure = buildInfo.Update ? structure->m_AccelerationStructure : VK_NULL_HANDLE;
		buildGeometryInfo.dstAccelerationStructure = structure->m_AccelerationStructure;
		buildGeometryInfo.geometryCount = static_cast<std::uint32_t>(structure->m_Geometries.size());
		buildGeometryInfo.pGeometries = structure->m_Geometries.data();
		buildGeometryInfo.scratchData.deviceAddress = structure->m_ScratchAddress;
		buildGeometryInfo.mode = buildInfo.Update
			? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
			: VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos(structure->m_PrimitiveCounts.size());

		for (std::size_t index = 0; index < rangeInfos.size(); ++index)
		{
			rangeInfos[index].primitiveCount = topLevel ? instanceCount : structure->m_PrimitiveCounts[index];
		}

		const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPointer = rangeInfos.data();

		if (structure->m_CompactedSizeQueryPool != VK_NULL_HANDLE)
		{
			vkCmdResetQueryPool(m_CommandBuffer, structure->m_CompactedSizeQueryPool, 0, 1);
		}

		m_Device->m_CmdBuildAccelerationStructures(m_CommandBuffer, 1, &buildGeometryInfo, &rangeInfoPointer);

		VkMemoryBarrier readBarrier = {};
		readBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		readBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		readBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			1,
			&readBarrier,
			0,
			nullptr,
			0,
			nullptr);

		if (structure->m_CompactedSizeQueryPool != VK_NULL_HANDLE)
		{
			m_Device->m_CmdWriteAccelerationStructuresProperties(
				m_CommandBuffer,
				1,
				&structure->m_AccelerationStructure,
				VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
				structure->m_CompactedSizeQueryPool,
				0);
		}

		retainResource(accelerationStructure);
		structure->m_Built = true;
		structure->m_BuiltInstanceCount = instanceCount;

		return {};
	}

	Status CommandList::compactAccelerationStructure(
		IAccelerationStructure& accelerationStructure)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not m_Device->m_RayTracingEnabled)
		{
			return ERR_UNSUPPORTED;
		}

		if (m_QueueType != QueueType::Graphics)
		{
			return ERR_UNSUPPORTED;
		}

		AccelerationStructure* structure = backendCast<AccelerationStructure>(accelerationStructure);

		if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		SPALL_TRY(validateAccelerationStructureCompaction(structure->m_Info));

		if ((not structure->m_Built) or structure->m_Compacted)
		{
			return ERR_INVALID_STATE;
		}

		VkDeviceSize compactedSize = 0;
		const VkResult queryResult = vkGetQueryPoolResults(
			m_Device->m_Device,
			structure->m_CompactedSizeQueryPool,
			0,
			1,
			sizeof(compactedSize),
			&compactedSize,
			sizeof(compactedSize),
			VK_QUERY_RESULT_64_BIT);

		if (queryResult == VK_NOT_READY)
		{
			return ERR_INVALID_STATE;
		}

		if (queryResult != VK_SUCCESS)
		{
			return mapVulkanStatus(queryResult);
		}

		if ((compactedSize == 0) or (compactedSize > structure->m_Info.Size))
		{
			return ERR_BACKEND_FAILURE;
		}

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkBufferCreateInfo storeCreateInfo = {};
		storeCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		storeCreateInfo.size = compactedSize;
		storeCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		storeCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer compactedBuffer = VK_NULL_HANDLE;
		VmaAllocation compactedAllocation = VK_NULL_HANDLE;
		VkResult vkResult = vmaCreateBuffer(m_Device->m_Allocator, &storeCreateInfo, &allocationCreateInfo, &compactedBuffer, &compactedAllocation, nullptr);

		if (vkResult != VK_SUCCESS)
		{
			return fail(mapVulkanStatus(vkResult));
		}

		VkAccelerationStructureCreateInfoKHR structureCreateInfo = {};
		structureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		structureCreateInfo.buffer = compactedBuffer;
		structureCreateInfo.offset = 0;
		structureCreateInfo.size = compactedSize;
		structureCreateInfo.type = vulkanAccelerationStructureType(structure->m_Info.Type);

		VkAccelerationStructureKHR compacted = VK_NULL_HANDLE;
		vkResult = m_Device->m_CreateAccelerationStructure(m_Device->m_Device, &structureCreateInfo, nullptr, &compacted);

		if (vkResult != VK_SUCCESS)
		{
			vmaDestroyBuffer(m_Device->m_Allocator, compactedBuffer, compactedAllocation);

			return fail(mapVulkanStatus(vkResult));
		}

		VkCopyAccelerationStructureInfoKHR copyInfo = {};
		copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
		copyInfo.src = structure->m_AccelerationStructure;
		copyInfo.dst = compacted;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

		m_Device->m_CmdCopyAccelerationStructure(m_CommandBuffer, &copyInfo);

		VkMemoryBarrier readBarrier = {};
		readBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		readBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		readBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			1,
			&readBarrier,
			0,
			nullptr,
			0,
			nullptr);

		m_RetiredAccelerationStructures.push_back(
			RetiredAccelerationStructure {structure->m_AccelerationStructure, structure->m_Buffer, structure->m_Allocation});

		structure->m_AccelerationStructure = compacted;
		structure->m_Buffer = compactedBuffer;
		structure->m_Allocation = compactedAllocation;
		structure->m_Info.Size = compactedSize;
		structure->m_Compacted = true;

		retainResource(accelerationStructure);

		return {};
	}

	Status CommandList::copyBuffer(
		IBuffer& destination,
		std::uint32_t destinationOffset,
		IBuffer& source,
		std::uint32_t sourceOffset,
		std::uint32_t size)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		error = validateCopyBufferArguments(destination, destinationOffset, source, sourceOffset, size);

		if (error != SUCCESS)
		{
			return error;
		}

		Buffer* destinationBuffer = backendCast<Buffer>(destination);
		Buffer* sourceBuffer = backendCast<Buffer>(source);

		if ((destinationBuffer == nullptr) or
			(sourceBuffer == nullptr) or
			(destinationBuffer->m_Device.get() != m_Device.get()) or
			(sourceBuffer->m_Device.get() != m_Device.get()) or
			((destinationBuffer->m_Info.Usage & BufferUsageFlags::TransferDestination) == BufferUsageFlags::None) or
			((sourceBuffer->m_Info.Usage & BufferUsageFlags::TransferSource) == BufferUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		retainResource(*destinationBuffer);
		retainResource(*sourceBuffer);

		error = requireBufferState(*sourceBuffer, ResourceStateFlags::CopySource);

		if (error == SUCCESS)
		{
			error = requireBufferState(*destinationBuffer, ResourceStateFlags::CopyDest);
		}

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		VkBufferCopy copyRegion = {};
		copyRegion.dstOffset = destinationOffset;
		copyRegion.srcOffset = sourceOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(m_CommandBuffer, sourceBuffer->m_Buffer, destinationBuffer->m_Buffer, 1, &copyRegion);

		return {};
	}

	Status CommandList::copyBufferToTexture(
		ITexture& destination,
		const TextureRegion& region,
		IBuffer& source,
		std::uint32_t sourceOffset,
		std::uint32_t sourceRowPitch)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Texture* destinationTexture = backendCast<Texture>(destination);
		Buffer* sourceBuffer = backendCast<Buffer>(source);

		if ((destinationTexture == nullptr) or
			(sourceBuffer == nullptr) or
			(destinationTexture->m_Device.get() != m_Device.get()) or
			(sourceBuffer->m_Device.get() != m_Device.get()) or
			((destinationTexture->m_Info.Usage & TextureUsageFlags::TransferDestination) == TextureUsageFlags::None) or
			((sourceBuffer->m_Info.Usage & BufferUsageFlags::TransferSource) == BufferUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		error = validateTextureBufferCopyArguments(destination, region, source, sourceOffset, sourceRowPitch);

		if (error != SUCCESS)
		{
			return error;
		}

		const std::optional<TextureFormatProperties> formatInfo = textureFormatInfo(destinationTexture->m_Info.Format);

		if (not formatInfo.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		if (((sourceOffset % formatInfo->bytesPerBlock) != 0) or
			((sourceRowPitch % formatInfo->bytesPerBlock) != 0))
		{
			return ERR_INVALID_RANGE;
		}

		retainResource(*destinationTexture);
		retainResource(*sourceBuffer);

		error = referencePresentTexture(destinationTexture);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireBufferState(*sourceBuffer, ResourceStateFlags::CopySource);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireTextureState(*destinationTexture, ResourceStateFlags::CopyDest);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = sourceOffset;
		copyRegion.bufferRowLength = (sourceRowPitch / formatInfo->bytesPerBlock) * formatInfo->blockWidth;
		copyRegion.imageSubresource.aspectMask = destinationTexture->m_AspectMask;
		const TextureRegion resolved = resolveTextureRegion(destinationTexture->m_Info, region);

		copyRegion.imageSubresource.mipLevel = resolved.MipLevel;
		copyRegion.imageSubresource.baseArrayLayer = resolved.ArrayLayer;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset.x = static_cast<std::int32_t>(resolved.X);
		copyRegion.imageOffset.y = static_cast<std::int32_t>(resolved.Y);
		copyRegion.imageOffset.z = static_cast<std::int32_t>(resolved.Z);
		copyRegion.imageExtent.width = resolved.Width;
		copyRegion.imageExtent.height = resolved.Height;
		copyRegion.imageExtent.depth = resolved.Depth;

		vkCmdCopyBufferToImage(m_CommandBuffer, sourceBuffer->m_Buffer, destinationTexture->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		return {};
	}

	Status CommandList::copyTextureToBuffer(
		IBuffer& destination,
		std::uint32_t destinationOffset,
		std::uint32_t destinationRowPitch,
		ITexture& source,
		const TextureRegion& region)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Buffer* destinationBuffer = backendCast<Buffer>(destination);
		Texture* sourceTexture = backendCast<Texture>(source);

		if ((destinationBuffer == nullptr) or (sourceTexture == nullptr) or
			(destinationBuffer->m_Device.get() != m_Device.get()) or (sourceTexture->m_Device.get() != m_Device.get()) or
			((destinationBuffer->m_Info.Usage & BufferUsageFlags::TransferDestination) == BufferUsageFlags::None) or
			((sourceTexture->m_Info.Usage & TextureUsageFlags::TransferSource) == TextureUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (destinationBuffer->m_Info.CpuAccess != MemoryAccess::Read)
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if (isDepthFormat(sourceTexture->m_Info.Format))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		error = validateTextureBufferCopyArguments(source, region, destination, destinationOffset, destinationRowPitch);

		if (error != SUCCESS)
		{
			return error;
		}

		const std::optional<TextureFormatProperties> formatInfo = textureFormatInfo(sourceTexture->m_Info.Format);

		if (not formatInfo.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		if (((destinationOffset % formatInfo->bytesPerBlock) != 0) or
			((destinationRowPitch % formatInfo->bytesPerBlock) != 0))
		{
			return ERR_INVALID_RANGE;
		}

		retainResource(*destinationBuffer);
		retainResource(*sourceTexture);

		error = referencePresentTexture(sourceTexture);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireTextureState(*sourceTexture, ResourceStateFlags::CopySource);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireBufferState(*destinationBuffer, ResourceStateFlags::CopyDest);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = destinationOffset;
		copyRegion.bufferRowLength = (destinationRowPitch / formatInfo->bytesPerBlock) * formatInfo->blockWidth;
		copyRegion.imageSubresource.aspectMask = sourceTexture->m_AspectMask;
		const TextureRegion resolved = resolveTextureRegion(sourceTexture->m_Info, region);

		copyRegion.imageSubresource.mipLevel = resolved.MipLevel;
		copyRegion.imageSubresource.baseArrayLayer = resolved.ArrayLayer;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset.x = static_cast<std::int32_t>(resolved.X);
		copyRegion.imageOffset.y = static_cast<std::int32_t>(resolved.Y);
		copyRegion.imageOffset.z = static_cast<std::int32_t>(resolved.Z);
		copyRegion.imageExtent.width = resolved.Width;
		copyRegion.imageExtent.height = resolved.Height;
		copyRegion.imageExtent.depth = resolved.Depth;

		vkCmdCopyImageToBuffer(
			m_CommandBuffer,
			sourceTexture->m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destinationBuffer->m_Buffer,
			1,
			&copyRegion);

		return {};
	}

	Status CommandList::copyTexture(
		ITexture& destination,
		ITexture& source)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Texture* destinationTexture = backendCast<Texture>(destination);
		Texture* sourceTexture = backendCast<Texture>(source);

		if ((destinationTexture == nullptr) or
			(sourceTexture == nullptr) or
			(destinationTexture->m_Device.get() != m_Device.get()) or
			(sourceTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateCopyTextureArguments(destination, source);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(*destinationTexture);
		retainResource(*sourceTexture);

		error = referencePresentTexture(destinationTexture);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = referencePresentTexture(sourceTexture);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireTextureState(*sourceTexture, ResourceStateFlags::CopySource);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireTextureState(*destinationTexture, ResourceStateFlags::CopyDest);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		std::vector<VkImageCopy> copyRegions(sourceTexture->m_Info.MipLevels);

		for (std::uint32_t mipLevel = 0; mipLevel < sourceTexture->m_Info.MipLevels; ++mipLevel)
		{
			VkImageCopy& copyRegion = copyRegions[mipLevel];
			copyRegion.srcSubresource.aspectMask = sourceTexture->m_AspectMask;
			copyRegion.srcSubresource.mipLevel = mipLevel;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.layerCount = sourceTexture->m_Info.ArrayLayers;
			copyRegion.dstSubresource.aspectMask = destinationTexture->m_AspectMask;
			copyRegion.dstSubresource.mipLevel = mipLevel;
			copyRegion.dstSubresource.baseArrayLayer = 0;
			copyRegion.dstSubresource.layerCount = destinationTexture->m_Info.ArrayLayers;
			copyRegion.extent.width = mipLevelExtent(sourceTexture->m_Info.Width, mipLevel);
			copyRegion.extent.height = mipLevelExtent(sourceTexture->m_Info.Height, mipLevel);
			copyRegion.extent.depth = mipLevelExtent(sourceTexture->m_Info.Depth, mipLevel);
		}

		vkCmdCopyImage(
			m_CommandBuffer,
			sourceTexture->m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destinationTexture->m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<std::uint32_t>(copyRegions.size()),
			copyRegions.data());

		return {};
	}

	Status CommandList::writeTimestamp(
		IQueryPool& queryPool,
		std::uint32_t query)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		QueryPool* vkQueryPool = backendCast<QueryPool>(queryPool);

		if ((vkQueryPool == nullptr) or (vkQueryPool->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateTimestampWrite(vkQueryPool->m_Info, query);

		if (error != SUCCESS)
		{
			return error;
		}

		for (const auto& timestampWrite : m_TimestampWrites)
		{
			if ((timestampWrite.first == vkQueryPool) and (timestampWrite.second == query))
			{
				return ERR_INVALID_STATE;
			}
		}

		retainResource(*vkQueryPool);
		m_TimestampWrites.emplace_back(vkQueryPool, query);

		vkCmdWriteTimestamp(m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkQueryPool->m_QueryPool, query);

		return {};
	}

	Status CommandList::recordQueryResets(
		VkCommandBuffer commandBuffer,
		bool* recordedResets) const
	{
		*recordedResets = false;

		for (const auto& timestampWrite : m_TimestampWrites)
		{
			vkCmdResetQueryPool(commandBuffer, timestampWrite.first->m_QueryPool, timestampWrite.second, 1);
			*recordedResets = true;
		}

		return {};
	}

	Status CommandList::generateMips(
		ITexture& texture)
	{
		Status error = spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true);

		if (error != SUCCESS)
		{
			return error;
		}

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Texture* vkTexture = backendCast<Texture>(texture);

		if ((vkTexture == nullptr) or (vkTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		error = validateGenerateMipsArguments(*vkTexture);

		if (error != SUCCESS)
		{
			return error;
		}

		const std::optional<VkFormat> format = toVkFormat(vkTexture->m_Info.Format);

		if (not format.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		VkFormatProperties properties = {};
		vkGetPhysicalDeviceFormatProperties(m_Device->m_PhysicalDevice, format.value(), &properties);

		constexpr VkFormatFeatureFlags requiredFeatures =
			VK_FORMAT_FEATURE_BLIT_SRC_BIT |
			VK_FORMAT_FEATURE_BLIT_DST_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

		if ((properties.optimalTilingFeatures & requiredFeatures) != requiredFeatures)
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		retainResource(*vkTexture);

		const ResourceStateFlags originalState = m_StateTracker.currentTextureState(*vkTexture);

		if (m_AutomaticBarriers)
		{
			error = m_StateTracker.requireTextureState(*vkTexture, ResourceStateFlags::CopyDest);
		}
		else
		{
			error = m_StateTracker.validateTextureState(*vkTexture, ResourceStateFlags::CopyDest);
		}

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		for (std::uint32_t mipLevel = 1; mipLevel < vkTexture->m_Info.MipLevels; ++mipLevel)
		{
			VkImageMemoryBarrier sourceBarrier = {};
			sourceBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			sourceBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			sourceBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			sourceBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			sourceBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			sourceBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			sourceBarrier.image = vkTexture->m_Image;
			sourceBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			sourceBarrier.subresourceRange.baseMipLevel = mipLevel - 1;
			sourceBarrier.subresourceRange.levelCount = 1;
			sourceBarrier.subresourceRange.baseArrayLayer = 0;
			sourceBarrier.subresourceRange.layerCount = vkTexture->m_Info.ArrayLayers;

			vkCmdPipelineBarrier(
				m_CommandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&sourceBarrier);

			VkImageBlit blit = {};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = mipLevel - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = vkTexture->m_Info.ArrayLayers;
			blit.srcOffsets[1].x = static_cast<std::int32_t>(mipLevelExtent(vkTexture->m_Info.Width, mipLevel - 1));
			blit.srcOffsets[1].y = static_cast<std::int32_t>(mipLevelExtent(vkTexture->m_Info.Height, mipLevel - 1));
			blit.srcOffsets[1].z = 1;
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = mipLevel;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = vkTexture->m_Info.ArrayLayers;
			blit.dstOffsets[1].x = static_cast<std::int32_t>(mipLevelExtent(vkTexture->m_Info.Width, mipLevel));
			blit.dstOffsets[1].y = static_cast<std::int32_t>(mipLevelExtent(vkTexture->m_Info.Height, mipLevel));
			blit.dstOffsets[1].z = 1;

			vkCmdBlitImage(
				m_CommandBuffer,
				vkTexture->m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				vkTexture->m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&blit,
				VK_FILTER_LINEAR);
		}

		VkImageMemoryBarrier normalizeBarrier = {};
		normalizeBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		normalizeBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		normalizeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		normalizeBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		normalizeBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		normalizeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		normalizeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		normalizeBarrier.image = vkTexture->m_Image;
		normalizeBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		normalizeBarrier.subresourceRange.baseMipLevel = 0;
		normalizeBarrier.subresourceRange.levelCount = vkTexture->m_Info.MipLevels - 1;
		normalizeBarrier.subresourceRange.baseArrayLayer = 0;
		normalizeBarrier.subresourceRange.layerCount = vkTexture->m_Info.ArrayLayers;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&normalizeBarrier);

		if (m_AutomaticBarriers and (originalState != ResourceStateFlags::CopyDest))
		{
			error = m_StateTracker.requireTextureState(*vkTexture, originalState);

			if (error != SUCCESS)
			{
				return fail(error);
			}

			error = m_StateTracker.commitBarriers();

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		return {};
	}
} // namespace spall::vk
