#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/TextureView/ITextureView.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

namespace spall::vk
{
	class Texture;
	class Device;
	class GraphicsQueue;
	class CommandList;
	class ResourceSet;

	class TextureView final : public SharedObject<ITextureView>
	{
	public:
		struct Subresources
		{
			TextureAspectFlags Aspects = TextureAspectFlags::None;
			std::uint32_t BaseMipLevel = 0;
			std::uint32_t MipLevels = 1;
			std::uint32_t BaseArrayLayer = 0;
			std::uint32_t ArrayLayers = 1;
			bool Cubemap = false;
		};

		TextureView(
			Texture& texture,
			const Subresources& subresources,
			VkImageView view,
			bool ownsView = true);

		~TextureView(void) override;

		RenderBackendType backendType(void) const override;
		ITexture& texture(void) const override;
		Format format(void) const override;
		TextureAspectFlags aspects(void) const override;
		std::uint32_t baseMipLevel(void) const override;
		std::uint32_t mipLevels(void) const override;
		std::uint32_t baseArrayLayer(void) const override;
		std::uint32_t arrayLayers(void) const override;
		bool isCubemap(void) const override;

	private:
		Resource<Texture> m_Texture;

		TextureAspectFlags m_Aspects = TextureAspectFlags::None;
		std::uint32_t m_BaseMipLevel = 0;
		std::uint32_t m_MipLevels = 1;
		std::uint32_t m_BaseArrayLayer = 0;
		std::uint32_t m_ArrayLayers = 1;
		bool m_Cubemap = false;

		VkImageView m_View = VK_NULL_HANDLE;
		bool m_OwnsView = true;

	private:
		friend class Device;
		friend class GraphicsQueue;
		friend class CommandList;
		friend class ResourceSet;
	};
} // namespace spall::vk
