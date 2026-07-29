#pragma once

#include <spall/Queue/IGraphicsQueue.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>

namespace spall::vk
{
	class Device;
	class SwapChain;
	class Frame;

	class GraphicsQueue final : public IGraphicsQueue
	{
	public:
		GraphicsQueue(
			Device& device,
			VkQueue nativeQueue,
			std::uint32_t familyIndex);

		~GraphicsQueue(void) override;

		Status initialize(void);

		RenderBackendType backendType(void) const override;

		Status acquireFrame(
			ISwapChain& swapChain,
			Resource<IFrame>* frame) override;

		Status submit(ICommandList& commandList) override;
		Status present(IFrame& frame) override;
		Status waitForQueue(IQueue& other) override;
		Status waitIdle(void) override;

		VkSemaphore timelineSemaphore(void) const;
		std::uint64_t timelineValue(void) const;

	private:
		Device* m_Device = nullptr;
		VkQueue m_NativeQueue = VK_NULL_HANDLE;
		std::uint32_t m_FamilyIndex = 0;

		VkSemaphore m_TimelineSemaphore = VK_NULL_HANDLE;
		std::uint64_t m_TimelineValue = 0;
		VkSemaphore m_PendingWaitSemaphore = VK_NULL_HANDLE;
		std::uint64_t m_PendingWaitValue = 0;

		SwapChain* m_ActiveSwapChain = nullptr;
		Frame* m_ActiveFrame = nullptr;
		bool m_ActiveFrameSubmitted = false;

	private:
		friend class Device;
		friend class SwapChain;
		friend class Frame;
	};
} // namespace spall::vk
