#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/Common/VK_BackendCast.h>
#include <src/Backends/Vulkan/Queue/VK_GraphicsQueue.h>

#include <src/Backends/Vulkan/CommandList/VK_CommandList.h>
#include <src/Backends/Vulkan/Common/VK_PresentSync.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Frame/VK_Frame.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChain.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChainGeneration.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>
#include <limits>
#include <memory>

namespace spall::vk
{
	GraphicsQueue::GraphicsQueue(
		Device& device,
		VkQueue nativeQueue,
		std::uint32_t familyIndex)
		: m_Device(&device), m_NativeQueue(nativeQueue), m_FamilyIndex(familyIndex)
	{
	}

	GraphicsQueue::~GraphicsQueue()
	{
		if ((m_Device != nullptr) and (m_Device->m_Device != VK_NULL_HANDLE) and (m_TimelineSemaphore != VK_NULL_HANDLE))
		{
			vkDestroySemaphore(m_Device->m_Device, m_TimelineSemaphore, nullptr);
		}
	}

	Status GraphicsQueue::initialize()
	{
		VkSemaphoreTypeCreateInfo timelineType = {};
		timelineType.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineType.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineType.initialValue = 0;

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = &timelineType;

		const VkResult vkResult = vkCreateSemaphore(m_Device->m_Device, &semaphoreInfo, nullptr, &m_TimelineSemaphore);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		return {};
	}

	RenderBackendType GraphicsQueue::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	VkSemaphore GraphicsQueue::timelineSemaphore() const
	{
		return m_TimelineSemaphore;
	}

	std::uint64_t GraphicsQueue::timelineValue() const
	{
		return m_TimelineValue;
	}

	Status GraphicsQueue::waitForQueue(
		IQueue& other)
	{
		GraphicsQueue* otherQueue = backendCast<GraphicsQueue>(other);

		if (otherQueue == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (otherQueue == this)
		{
			return {};
		}

		m_PendingWaitSemaphore = otherQueue->m_TimelineSemaphore;
		m_PendingWaitValue = otherQueue->m_TimelineValue;

		return {};
	}

	Status GraphicsQueue::acquireFrame(
		ISwapChain& swapChain,
		Resource<IFrame>* frame)
	{
		if (frame == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (m_ActiveFrame != nullptr)
		{
			return ERR_INVALID_STATE;
		}

		SwapChain* backendSwapChain = backendCast<SwapChain>(swapChain);

		if (backendSwapChain == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendSwapChain->m_Device.get() != m_Device)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((backendSwapChain->m_Width == 0) or (backendSwapChain->m_Height == 0))
		{
			return ERR_INVALID_SIZE;
		}

		for (std::uint32_t acquireAttempt = 0; acquireAttempt < 2; ++acquireAttempt)
		{
			if (backendSwapChain->m_RecreateBeforeAcquire or (not backendSwapChain->m_Generation) or
				backendSwapChain->m_FrameResources.empty() or backendSwapChain->m_BackBuffers.empty())
			{
				SPALL_TRY(backendSwapChain->recreate(backendSwapChain->m_Width, backendSwapChain->m_Height));
			}

			const std::uint32_t frameSlotIndex = backendSwapChain->m_CurrentFrameSlot % static_cast<std::uint32_t>(backendSwapChain->m_FrameResources.size());
			SwapChain::FrameResources& frameResources = backendSwapChain->m_FrameResources[frameSlotIndex];

			SPALL_ASSERT(static_cast<bool>(frameResources.InFlightCommandList) == (frameResources.InFlightSubmissionSerial != 0));

			if (frameResources.InFlightCommandList)
			{
				SPALL_TRY(frameResources.InFlightCommandList->waitForSubmission(frameResources.InFlightSubmissionSerial));

				frameResources.InFlightCommandList.reset();
				frameResources.InFlightSubmissionSerial = 0;
			}

			SPALL_ASSERT(backendSwapChain->m_Generation);

			std::uint32_t imageIndex = 0;
			const VkResult vkResult = vkAcquireNextImageKHR(
				m_Device->m_Device,
				backendSwapChain->m_Generation->m_SwapChain,
				UINT64_MAX,
				frameResources.ImageAvailableSemaphore,
				VK_NULL_HANDLE,
				&imageIndex);

			if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
			{
				backendSwapChain->m_RecreateBeforeAcquire = true;

				if (acquireAttempt == 0)
				{
					continue;
				}

				return ERR_SWAP_CHAIN_OUT_OF_DATE;
			}

			if ((vkResult != VK_SUCCESS) and (vkResult != VK_SUBOPTIMAL_KHR))
			{
				return mapVulkanStatus(vkResult);
			}

			if (vkResult == VK_SUBOPTIMAL_KHR)
			{
				backendSwapChain->m_RecreateBeforeAcquire = true;
			}

			if (imageIndex >= backendSwapChain->m_BackBuffers.size())
			{
				backendSwapChain->m_RecreateBeforeAcquire = true;
				return ERR_SWAP_CHAIN_ACQUIRE_FAILED;
			}

			std::unique_ptr<Frame> vkFrame = std::make_unique<Frame>(
				*backendSwapChain,
				frameSlotIndex,
				imageIndex,
				*backendSwapChain->m_BackBuffers[imageIndex].Texture,
				*backendSwapChain->m_BackBuffers[imageIndex].View);

			m_ActiveSwapChain = backendSwapChain;
			m_ActiveFrame = vkFrame.get();
			m_ActiveFrameSubmitted = false;

			*frame = Resource<IFrame>(vkFrame.release());

			return {};
		}

		return ERR_SWAP_CHAIN_ACQUIRE_FAILED;
	}

	Status GraphicsQueue::submit(
		ICommandList& commandList)
	{
		CommandList* backendCommandList = backendCast<CommandList>(commandList);

		if (backendCommandList == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendCommandList->m_Device.get() != m_Device)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendCommandList->m_ExecutionState != CommandList::ExecutionState::Executable)
		{
			return ERR_INVALID_STATE;
		}

		if ((backendCommandList->m_CommandBuffer == VK_NULL_HANDLE) or
			(backendCommandList->m_EntryCommandBuffer == VK_NULL_HANDLE) or
			(backendCommandList->m_SubmissionFence == VK_NULL_HANDLE))
		{
			return ERR_INVALID_STATE;
		}

		const VkCommandBuffer submittedCommandBuffers[2] = {
			backendCommandList->m_EntryCommandBuffer,
			backendCommandList->m_CommandBuffer};

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 2;
		submitInfo.pCommandBuffers = submittedCommandBuffers;

		VkSemaphore waitSemaphores[2] = {};
		VkPipelineStageFlags waitStages[2] = {};
		std::uint64_t waitValues[2] = {};
		std::uint32_t waitCount = 0;

		if (m_PendingWaitSemaphore != VK_NULL_HANDLE)
		{
			waitSemaphores[waitCount] = m_PendingWaitSemaphore;
			waitStages[waitCount] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			waitValues[waitCount] = m_PendingWaitValue;
			++waitCount;
		}

		const std::uint64_t timelineSignalValue = m_TimelineValue + 1;

		VkSemaphore signalSemaphores[2] = {};
		std::uint64_t signalValues[2] = {};
		std::uint32_t signalCount = 0;

		signalSemaphores[signalCount] = m_TimelineSemaphore;
		signalValues[signalCount] = timelineSignalValue;
		++signalCount;

		SwapChain::FrameResources* submittedFrameResources = nullptr;

		if (backendCommandList->m_ReferencedPresentTexture)
		{
			if ((m_ActiveFrame == nullptr) or (m_ActiveSwapChain == nullptr))
			{
				return ERR_INVALID_STATE;
			}

			if (m_ActiveFrameSubmitted)
			{
				return ERR_INVALID_STATE;
			}

			if ((backendCommandList->m_ReferencedSwapChain != m_ActiveSwapChain) or
				(backendCommandList->m_ReferencedPresentTexture.get() != m_ActiveFrame->m_PresentTexture))
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			SwapChain::FrameResources& frameResources = m_ActiveSwapChain->m_FrameResources[m_ActiveFrame->m_FrameSlotIndex];
			SwapChain::BackBuffer& backBuffer = m_ActiveSwapChain->m_BackBuffers[m_ActiveFrame->m_ImageIndex];

			waitSemaphores[waitCount] = frameResources.ImageAvailableSemaphore;
			waitStages[waitCount] = SwapChainAcquireWaitStages;
			waitValues[waitCount] = 0;
			++waitCount;

			signalSemaphores[signalCount] = backBuffer.RenderFinishedSemaphore;
			signalValues[signalCount] = 0;
			++signalCount;

			submittedFrameResources = &frameResources;
		}

		VkTimelineSemaphoreSubmitInfo timelineInfo = {};
		timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timelineInfo.waitSemaphoreValueCount = waitCount;
		timelineInfo.pWaitSemaphoreValues = waitValues;
		timelineInfo.signalSemaphoreValueCount = signalCount;
		timelineInfo.pSignalSemaphoreValues = signalValues;

		submitInfo.pNext = &timelineInfo;
		submitInfo.waitSemaphoreCount = waitCount;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = signalCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		SPALL_ASSERT(backendCommandList->m_SubmissionSerial < (std::numeric_limits<std::uint64_t>::max)());

		if (backendCommandList->m_SubmissionSerial == (std::numeric_limits<std::uint64_t>::max)())
		{
			return ERR_INVALID_STATE;
		}

		Status error = backendCommandList->m_StateTracker.validateSubmissionStates();

		if (error != SUCCESS)
		{
			return error;
		}

		VkCommandBufferBeginInfo entryBeginInfo = {};
		entryBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		entryBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult entryResult = vkBeginCommandBuffer(backendCommandList->m_EntryCommandBuffer, &entryBeginInfo);

		if (entryResult != VK_SUCCESS)
		{
			return mapVulkanStatus(entryResult);
		}

		bool entryCommandsRecorded = false;
		error = backendCommandList->recordQueryResets(backendCommandList->m_EntryCommandBuffer, &entryCommandsRecorded);

		if (error != SUCCESS)
		{
			vkEndCommandBuffer(backendCommandList->m_EntryCommandBuffer);

			return error;
		}

		bool entryBarriersRecorded = false;
		error = backendCommandList->m_StateTracker.recordEntryBarriers(backendCommandList->m_EntryCommandBuffer, &entryBarriersRecorded);

		if (error != SUCCESS)
		{
			vkEndCommandBuffer(backendCommandList->m_EntryCommandBuffer);

			return error;
		}

		entryCommandsRecorded = entryCommandsRecorded or entryBarriersRecorded;
		entryResult = vkEndCommandBuffer(backendCommandList->m_EntryCommandBuffer);

		if (entryResult != VK_SUCCESS)
		{
			return mapVulkanStatus(entryResult);
		}

		if (not entryCommandsRecorded)
		{
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &submittedCommandBuffers[1];
		}

		const std::uint64_t submissionSerial = backendCommandList->m_SubmissionSerial + 1;
		VkResult vkResult = vkResetFences(m_Device->m_Device, 1, &backendCommandList->m_SubmissionFence);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		vkResult = vkQueueSubmit(m_NativeQueue, 1, &submitInfo, backendCommandList->m_SubmissionFence);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		backendCommandList->m_StateTracker.commandListSubmitted();
		backendCommandList->m_SubmissionSerial = submissionSerial;
		backendCommandList->m_ExecutionState = CommandList::ExecutionState::Pending;

		m_TimelineValue = timelineSignalValue;
		m_PendingWaitSemaphore = VK_NULL_HANDLE;
		m_PendingWaitValue = 0;

		if (submittedFrameResources != nullptr)
		{
			submittedFrameResources->InFlightCommandList.reset(backendCommandList);
			submittedFrameResources->InFlightSubmissionSerial = submissionSerial;
			m_ActiveFrameSubmitted = true;
		}

		return {};
	}

	Status GraphicsQueue::present(
		IFrame& frame)
	{
		Frame* backendFrame = backendCast<Frame>(frame);

		if (backendFrame == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((m_ActiveFrame == nullptr) or (m_ActiveSwapChain == nullptr) or (m_ActiveFrame != backendFrame))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (not m_ActiveFrameSubmitted)
		{
			return ERR_INVALID_STATE;
		}

		SPALL_ASSERT(m_ActiveSwapChain->m_Generation);

		SwapChain::BackBuffer& backBuffer = m_ActiveSwapChain->m_BackBuffers[backendFrame->m_ImageIndex];
		const VkSwapchainKHR swapChains[] = {m_ActiveSwapChain->m_Generation->m_SwapChain};
		const std::uint32_t imageIndices[] = {backendFrame->m_ImageIndex};

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &backBuffer.RenderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = imageIndices;

		const VkResult vkResult = vkQueuePresentKHR(m_NativeQueue, &presentInfo);
		SwapChain* presentedSwapChain = backendFrame->m_SwapChain.get();

		m_ActiveFrame = nullptr;
		m_ActiveSwapChain = nullptr;
		m_ActiveFrameSubmitted = false;

		if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			presentedSwapChain->m_RecreateBeforeAcquire = true;
			return ERR_SWAP_CHAIN_OUT_OF_DATE;
		}

		if ((vkResult != VK_SUCCESS) and (vkResult != VK_SUBOPTIMAL_KHR))
		{
			return mapVulkanStatus(vkResult);
		}

		if (vkResult == VK_SUBOPTIMAL_KHR)
		{
			presentedSwapChain->m_RecreateBeforeAcquire = true;
		}

		if (not presentedSwapChain->m_FrameResources.empty())
		{
			presentedSwapChain->m_CurrentFrameSlot = (backendFrame->m_FrameSlotIndex + 1) % static_cast<std::uint32_t>(presentedSwapChain->m_FrameResources.size());
		}

		return {};
	}

	Status GraphicsQueue::waitIdle()
	{
		const VkResult vkResult = vkQueueWaitIdle(m_NativeQueue);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		return {};
	}
} // namespace spall::vk
