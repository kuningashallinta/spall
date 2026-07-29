#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>

namespace spall::vk
{
	TextureView::TextureView(
		Texture& texture,
		const Subresources& subresources,
		VkImageView view,
		bool ownsView)
		: m_Texture(&texture),
		  m_Aspects(subresources.Aspects),
		  m_BaseMipLevel(subresources.BaseMipLevel),
		  m_MipLevels(subresources.MipLevels),
		  m_BaseArrayLayer(subresources.BaseArrayLayer),
		  m_ArrayLayers(subresources.ArrayLayers),
		  m_Cubemap(subresources.Cubemap),
		  m_View(view),
		  m_OwnsView(ownsView)
	{
	}

	TextureView::~TextureView()
	{
		if ((not m_Texture) or (not m_Texture->m_Device) or not m_OwnsView)
		{
			return;
		}

		if (m_View != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Texture->m_Device->m_Device, m_View, nullptr);
		}
	}

	RenderBackendType TextureView::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	ITexture& TextureView::texture() const
	{
		return *m_Texture;
	}

	Format TextureView::format() const
	{
		return m_Texture->m_Info.Format;
	}

	TextureAspectFlags TextureView::aspects() const
	{
		return m_Aspects;
	}

	std::uint32_t TextureView::baseMipLevel() const
	{
		return m_BaseMipLevel;
	}

	std::uint32_t TextureView::mipLevels() const
	{
		return m_MipLevels;
	}

	std::uint32_t TextureView::baseArrayLayer() const
	{
		return m_BaseArrayLayer;
	}

	std::uint32_t TextureView::arrayLayers() const
	{
		return m_ArrayLayers;
	}

	bool TextureView::isCubemap() const
	{
		return m_Cubemap;
	}
} // namespace spall::vk
