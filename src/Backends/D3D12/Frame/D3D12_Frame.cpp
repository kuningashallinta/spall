// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Frame/D3D12_Frame.h>

#include <spall/Common/Assert.h>
#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>
#include <src/Backends/D3D12/SwapChain/D3D12_SwapChain.h>

#include <cstdint>
#include <limits>

namespace spall::d3d12
{
	Frame::Frame(
		SwapChain& swapChain,
		std::uint32_t index,
		Texture& texture,
		TextureView& textureView)
		: m_SwapChain(&swapChain), m_Index(index), m_PresentTexture(&texture), m_PresentTextureView(&textureView)
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
			queue.m_ActiveFrame = nullptr;
			queue.m_ActiveSwapChain = nullptr;
			queue.m_ActiveFrameSubmitted = false;
		}

		SPALL_VERIFY(m_SwapChain->m_LiveFrameCount != 0);
		--m_SwapChain->m_LiveFrameCount;
	}

	RenderBackendType Frame::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	std::uint32_t Frame::index() const
	{
		return m_Index;
	}

	ITexture& Frame::presentTexture()
	{
		return *m_PresentTexture;
	}

	ITextureView& Frame::presentTextureView()
	{
		return *m_PresentTextureView;
	}
} // namespace spall::d3d12
