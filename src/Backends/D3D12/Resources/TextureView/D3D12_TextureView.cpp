// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>

namespace spall::d3d12
{
	TextureView::TextureView(
		ITexture& texture,
		const Subresources& subresources,
		std::uint32_t renderTargetViewIndex,
		std::uint32_t depthStencilViewIndex)
		: m_Texture(&texture),
		  m_Storage(textureStorage(texture)),
		  m_Aspects(subresources.Aspects),
		  m_BaseMipLevel(subresources.BaseMipLevel),
		  m_MipLevels(subresources.MipLevels),
		  m_BaseArrayLayer(subresources.BaseArrayLayer),
		  m_ArrayLayers(subresources.ArrayLayers),
		  m_Cubemap(subresources.Cubemap),
		  m_RenderTargetViewIndex(renderTargetViewIndex),
		  m_DepthStencilViewIndex(depthStencilViewIndex)
	{
	}

	TextureView::~TextureView()
	{
		if (m_RenderTargetViewIndex != InvalidDescriptorIndex)
		{
			m_Storage->m_Device->m_RenderTargetViews.release(m_RenderTargetViewIndex);
		}

		if (m_DepthStencilViewIndex != InvalidDescriptorIndex)
		{
			m_Storage->m_Device->m_DepthStencilViews.release(m_DepthStencilViewIndex);
		}
	}

	RenderBackendType TextureView::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	ITexture& TextureView::texture() const
	{
		return *m_Texture;
	}

	Format TextureView::format() const
	{
		return m_Storage->m_Info.Format;
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
} // namespace spall::d3d12
