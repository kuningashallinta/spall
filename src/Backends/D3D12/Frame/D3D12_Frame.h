#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Frame/IFrame.h>

#include <cstdint>

namespace spall::d3d12
{
	class GraphicsQueue;
	class SwapChain;
	class Texture;
	class TextureView;

	class Frame final : public SharedObject<IFrame>
	{
	public:
		Frame(
			SwapChain& swapChain,
			std::uint32_t index,
			Texture& texture,
			TextureView& textureView);

		~Frame(void) override;

		RenderBackendType backendType(void) const override;
		std::uint32_t index(void) const override;
		ITexture& presentTexture(void) override;
		ITextureView& presentTextureView(void) override;

	private:
		Resource<SwapChain> m_SwapChain;

		std::uint32_t m_Index = 0;

		Texture* m_PresentTexture = nullptr;
		TextureView* m_PresentTextureView = nullptr;

	private:
		friend class GraphicsQueue;
	};
} // namespace spall::d3d12
