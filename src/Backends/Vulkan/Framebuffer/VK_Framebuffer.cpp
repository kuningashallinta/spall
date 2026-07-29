#include <src/Backends/Vulkan/Framebuffer/VK_Framebuffer.h>

#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>

namespace spall::vk
{
	Framebuffer::Framebuffer(
		Device& device,
		const FramebufferInfo& info,
		TextureView* colorViews[MaxColorAttachments],
		std::uint32_t colorCount,
		TextureView* depthView,
		TextureView* resolveViews[MaxColorAttachments])
		: m_Device(&device), m_Info(info), m_ColorCount(colorCount)
	{
		for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorCount; ++attachmentIndex)
		{
			m_ColorViews[attachmentIndex] = Resource<TextureView>(colorViews[attachmentIndex]);
		}

		if (depthView != nullptr)
		{
			m_DepthView = Resource<TextureView>(depthView);
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorCount; ++attachmentIndex)
		{
			if (resolveViews[attachmentIndex] != nullptr)
			{
				m_ResolveViews[attachmentIndex] = Resource<TextureView>(resolveViews[attachmentIndex]);
			}
		}
	}

	Framebuffer::~Framebuffer()
	{
		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE) and (m_Framebuffer != VK_NULL_HANDLE))
		{
			vkDestroyFramebuffer(m_Device->m_Device, m_Framebuffer, nullptr);
		}
	}

	RenderBackendType Framebuffer::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	FramebufferInfo Framebuffer::info() const
	{
		return m_Info;
	}
} // namespace spall::vk
