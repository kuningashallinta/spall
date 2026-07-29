#include <src/Backends/Vulkan/Frame/VK_Frame.h>

#include <spall/Common/Assert.h>
#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Queue/VK_GraphicsQueue.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChain.h>

#include <cstdint>
#include <limits>

namespace spall::vk
{
	Frame::Frame(
		SwapChain& swapChain,
		std::uint32_t frameSlotIndex,
		std::uint32_t imageIndex,
		Texture& presentTexture,
		TextureView& presentTextureView)
		: m_SwapChain(&swapChain), m_FrameSlotIndex(frameSlotIndex), m_ImageIndex(imageIndex), m_PresentTexture(&presentTexture), m_PresentTextureView(&presentTextureView)
	{
		SPALL_ASSERT(m_SwapChain->m_LiveFrameCount < (std::numeric_limits<std::uint32_t>::max)());
		++m_SwapChain->m_LiveFrameCount;
	}

	Frame::~Frame()
	{
		GraphicsQueue& queue = static_cast<GraphicsQueue&>(m_SwapChain->m_Device->graphicsQueue());

		if (queue.m_ActiveFrame == this)
		{
			SPALL_ASSERT(queue.m_ActiveSwapChain == m_SwapChain.get());
			m_SwapChain->m_RecreateBeforeAcquire = true;
			queue.m_ActiveSwapChain = nullptr;
			queue.m_ActiveFrame = nullptr;
			queue.m_ActiveFrameSubmitted = false;
		}

		SPALL_ASSERT(m_SwapChain->m_LiveFrameCount != 0);
		--m_SwapChain->m_LiveFrameCount;
	}

	RenderBackendType Frame::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	std::uint32_t Frame::index() const
	{
		return m_ImageIndex;
	}

	ITexture& Frame::presentTexture()
	{
		return *m_PresentTexture;
	}

	ITextureView& Frame::presentTextureView()
	{
		return *m_PresentTextureView;
	}
} // namespace spall::vk
