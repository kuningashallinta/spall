// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PresentMode.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/SwapChain/ISwapChain.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <vector>

namespace spall::d3d12
{
	class Device;
	class Frame;
	class GraphicsQueue;
	class Texture;
	class Texture2D;
	class TextureView;

	class SwapChain final : public SharedObject<ISwapChain>
	{
	public:
		struct BackBuffer
		{
			Resource<spall::d3d12::Texture2D> Texture;
			Resource<spall::d3d12::TextureView> View;

			/// Queue fence value that retires the work last submitted against this back buffer.
			std::uint64_t FenceValue = 0;
		};

		SwapChain(
			Device& device,
			ComPtr<IDXGISwapChain3> swapChain,
			std::uint32_t width,
			std::uint32_t height,
			Format format,
			PresentMode presentMode,
			std::uint32_t bufferCount);

		~SwapChain(void) override;

		RenderBackendType backendType(void) const override;
		Format format(void) const override;
		std::uint32_t frameCount(void) const override;

		Status resize(
			std::uint32_t width,
			std::uint32_t height) override;

		Status recreateBackBuffers(void);
		void releaseBackBuffers(void);
		std::uint32_t currentBackBufferIndex(void) const;

	private:
		Resource<Device> m_Device;

		ComPtr<IDXGISwapChain3> m_SwapChain;

		ComPtr<IDCompositionDevice> m_CompositionDevice;
		ComPtr<IDCompositionTarget> m_CompositionTarget;
		ComPtr<IDCompositionVisual> m_CompositionVisual;

		std::vector<BackBuffer> m_BackBuffers;

		std::uint32_t m_Width = 0;
		std::uint32_t m_Height = 0;
		std::uint32_t m_BufferCount = 0;
		std::uint32_t m_LiveFrameCount = 0;

		Format m_Format = Format::Unknown;
		PresentMode m_PresentMode = PresentMode::VSync;
		bool m_Occluded = false;

	private:
		friend class Device;
		friend class Frame;
		friend class GraphicsQueue;
	};
} // namespace spall::d3d12
