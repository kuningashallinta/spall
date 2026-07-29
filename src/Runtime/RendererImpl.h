#pragma once

#include <spall/Backend/IBackend.h>
#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/IDevice.h>
#include <spall/Frame/IFrame.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <spall/SwapChain/ISwapChain.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace spall
{
	class RendererImpl
	{
	public:
		RendererImpl(
			std::unique_ptr<IBackend> backend,
			Resource<IDevice> device,
			Resource<ISwapChain> swapChain,
			std::uint32_t width,
			std::uint32_t height,
			Format depthStencilFormat);

		Status createDepthAttachment(void);
		void releaseFramebuffers(void);

		Status framebuffer(
			IFrame& frame,
			Resource<IFramebuffer>* framebuffer);

		Status commandList(
			std::uint32_t frameIndex,
			Resource<ICommandList>* commandList);

		std::unique_ptr<IBackend> Backend;
		Resource<IDevice> Device;
		Resource<ISwapChain> SwapChain;
		Resource<ITexture> DepthTexture;
		Resource<ITextureView> DepthView;
		std::vector<Resource<IFramebuffer>> Framebuffers;
		std::vector<Resource<ICommandList>> CommandLists;
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
		Format DepthStencilFormat = Format::Unknown;
		bool FrameActive = false;
	};
} // namespace spall
