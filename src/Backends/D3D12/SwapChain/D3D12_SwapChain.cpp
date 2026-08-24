// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/SwapChain/D3D12_SwapChain.h>

#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>
#include <utility>

namespace spall::d3d12
{
	SwapChain::SwapChain(
		Device& device,
		ComPtr<IDXGISwapChain3> swapChain,
		std::uint32_t width,
		std::uint32_t height,
		Format format,
		PresentMode presentMode,
		std::uint32_t bufferCount)
		: m_Device(&device), m_SwapChain(std::move(swapChain)), m_Width(width), m_Height(height),
		  m_BufferCount(bufferCount), m_Format(format), m_PresentMode(presentMode)
	{
	}

	SwapChain::~SwapChain()
	{
		SPALL_VERIFY(m_LiveFrameCount == 0);
	}

	RenderBackendType SwapChain::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	Format SwapChain::format() const
	{
		return m_Format;
	}

	std::uint32_t SwapChain::frameCount() const
	{
		return m_BufferCount;
	}

	Status SwapChain::resize(
		std::uint32_t width,
		std::uint32_t height)
	{
		if (m_LiveFrameCount != 0)
		{
			return ERR_INVALID_STATE;
		}

		if ((width == 0) or (height == 0))
		{
			m_Width = width;
			m_Height = height;
			return {};
		}

		SPALL_TRY(m_Device->m_GraphicsQueue->waitIdle());

		for (const BackBuffer& backBuffer : m_BackBuffers)
		{
			const std::uint32_t expectedTextureReferences = backBuffer.View ? 2u : 1u;

			if ((backBuffer.Texture and (backBuffer.Texture->referenceCount() != expectedTextureReferences)) or
				(backBuffer.View and (backBuffer.View->referenceCount() != 1)))
			{
				return ERR_INVALID_STATE;
			}
		}

		releaseBackBuffers();

		const UINT flags = (m_PresentMode == PresentMode::Immediate) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		const HRESULT hr = m_SwapChain->ResizeBuffers(m_BufferCount, width, height, swapChainFormat(m_Format), flags);

		if (FAILED(hr))
		{
			const Status resizeError = mapStatus(hr);
			const Status recoveryError = recreateBackBuffers();

			if (recoveryError != SUCCESS)
			{
				return recoveryError;
			}

			return resizeError;
		}

		m_Width = width;
		m_Height = height;
		return recreateBackBuffers();
	}

	void SwapChain::releaseBackBuffers()
	{
		m_BackBuffers.clear();
	}

	Status SwapChain::recreateBackBuffers()
	{
		if (m_LiveFrameCount != 0)
		{
			return ERR_INVALID_STATE;
		}

		releaseBackBuffers();
		m_BackBuffers.resize(m_BufferCount);

		for (std::uint32_t bufferIndex = 0; bufferIndex < m_BufferCount; ++bufferIndex)
		{
			ComPtr<ID3D12Resource> backBuffer;
			HRESULT hr = m_SwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&backBuffer));

			if (FAILED(hr))
			{
				releaseBackBuffers();

				return mapStatus(hr);
			}

			TextureInfo textureInfo = {};
			textureInfo.Width = m_Width;
			textureInfo.Height = m_Height;
			textureInfo.Format = m_Format;
			textureInfo.Usage = TextureUsageFlags::ColorAttachment | TextureUsageFlags::TransferDestination;
			textureInfo.InitialState = ResourceStateFlags::Present;

			BackBuffer backBufferResources = {};
			backBufferResources.Texture = Resource<Texture>(new Texture(*m_Device, textureInfo, std::move(backBuffer), this));

			std::uint32_t renderTargetViewIndex = InvalidDescriptorIndex;
			Status error = m_Device->m_RenderTargetViews.allocate(&renderTargetViewIndex);

			if (error != SUCCESS)
			{
				releaseBackBuffers();

				return error;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = d3d12::format(m_Format);
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;
			viewDesc.Texture2D.PlaneSlice = 0;

			m_Device->m_Device->CreateRenderTargetView(
				backBufferResources.Texture->m_Resource.Get(),
				&viewDesc,
				m_Device->m_RenderTargetViews.cpuHandle(renderTargetViewIndex));

			TextureView::Subresources subresources = {};
			subresources.Aspects = TextureAspectFlags::Color;

			backBufferResources.View = Resource<TextureView>(
				new TextureView(*backBufferResources.Texture, subresources, renderTargetViewIndex));

			m_BackBuffers[bufferIndex] = std::move(backBufferResources);
		}

		return {};
	}

	std::uint32_t SwapChain::currentBackBufferIndex() const
	{
		SPALL_ASSERT(m_SwapChain != nullptr);
		return m_SwapChain->GetCurrentBackBufferIndex();
	}
} // namespace spall::d3d12
