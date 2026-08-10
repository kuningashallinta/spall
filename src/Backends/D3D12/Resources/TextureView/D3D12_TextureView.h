// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/TextureView/ITextureView.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>

namespace spall::d3d12
{
	class CommandList;
	class Device;
	class Frame;
	class Framebuffer;
	class GraphicsQueue;
	class Texture;

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
			std::uint32_t renderTargetViewIndex,
			std::uint32_t depthStencilViewIndex = InvalidDescriptorIndex);

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

		std::uint32_t m_RenderTargetViewIndex = InvalidDescriptorIndex;
		std::uint32_t m_DepthStencilViewIndex = InvalidDescriptorIndex;

	private:
		friend class CommandList;
		friend class Device;
		friend class Frame;
		friend class Framebuffer;
		friend class GraphicsQueue;
		friend class ResourceSet;
	};
} // namespace spall::d3d12
