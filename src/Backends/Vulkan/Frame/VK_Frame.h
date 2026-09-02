// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Frame/IFrame.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>

namespace spall::vk
{
	class SwapChain;
	class Texture2D;
	class TextureView;
	class GraphicsQueue;

	class Frame final : public SharedObject<IFrame>
	{
	public:
		Frame(
			SwapChain& swapChain,
			std::uint32_t frameSlotIndex,
			std::uint32_t imageIndex,
			Texture2D& presentTexture,
			TextureView& presentTextureView);

		~Frame(void) override;

		RenderBackendType backendType(void) const override;
		std::uint32_t index(void) const override;
		ITexture& presentTexture(void) override;
		ITextureView& presentTextureView(void) override;

	private:
		Resource<SwapChain> m_SwapChain;

		std::uint32_t m_FrameSlotIndex = 0;
		std::uint32_t m_ImageIndex = 0;

		Texture2D* m_PresentTexture = nullptr;
		TextureView* m_PresentTextureView = nullptr;

	private:
		friend class GraphicsQueue;
	};
} // namespace spall::vk
