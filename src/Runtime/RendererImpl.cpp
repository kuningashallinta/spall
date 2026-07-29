#include <src/Runtime/RendererImpl.h>

#include <spall/Device/IResourceFactory.h>
#include <spall/Framebuffer/FramebufferCreateInfo.h>
#include <spall/Resources/Texture/TextureCreateInfo.h>
#include <spall/Resources/TextureView/TextureViewCreateInfo.h>
#include <src/Validation/Common/FormatValidation.h>

#include <utility>

namespace spall
{
	RendererImpl::RendererImpl(
		std::unique_ptr<IBackend> backend,
		Resource<IDevice> device,
		Resource<ISwapChain> swapChain,
		std::uint32_t width,
		std::uint32_t height,
		Format depthStencilFormat)
		: Backend(std::move(backend)), Device(std::move(device)), SwapChain(std::move(swapChain)), Width(width), Height(height), DepthStencilFormat(depthStencilFormat)
	{
		Framebuffers.resize(SwapChain->frameCount());
		CommandLists.resize(SwapChain->frameCount());
	}

	Status RendererImpl::createDepthAttachment(
		void)
	{
		if ((DepthStencilFormat == Format::Unknown) or (Width == 0) or (Height == 0))
		{
			return {};
		}

		TextureCreateInfo textureInfo = {};
		textureInfo.Width = Width;
		textureInfo.Height = Height;
		textureInfo.Format = DepthStencilFormat;
		textureInfo.Usage = TextureUsageFlags::DepthStencilAttachment;

		Status status = Device->resources().createTexture(textureInfo, &DepthTexture);

		if (status != SUCCESS)
		{
			return status;
		}

		TextureViewCreateInfo viewInfo = {};
		viewInfo.Texture = DepthTexture.get();
		viewInfo.Aspects = TextureAspectFlags::Depth;

		if (hasStencilAspect(DepthStencilFormat))
		{
			viewInfo.Aspects |= TextureAspectFlags::Stencil;
		}

		status = Device->resources().createTextureView(viewInfo, &DepthView);

		if (status != SUCCESS)
		{
			DepthTexture.reset();
		}

		return status;
	}

	void RendererImpl::releaseFramebuffers(
		void)
	{
		Framebuffers.clear();
		DepthView.reset();
		DepthTexture.reset();
	}

	Status RendererImpl::framebuffer(
		IFrame& frame,
		Resource<IFramebuffer>* framebuffer)
	{
		if (framebuffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		const std::uint32_t frameIndex = frame.index();

		if (frameIndex >= Framebuffers.size())
		{
			return ERR_INVALID_RANGE;
		}

		if (Framebuffers[frameIndex])
		{
			*framebuffer = Framebuffers[frameIndex];
			return {};
		}

		FramebufferCreateInfo info = {};
		info.ColorAttachments[0] = &frame.presentTextureView();
		info.ColorAttachmentCount = 1;
		info.DepthAttachment = DepthView.get();

		Status status = Device->resources().createFramebuffer(info, &Framebuffers[frameIndex]);

		if (status == SUCCESS)
		{
			*framebuffer = Framebuffers[frameIndex];
		}

		return status;
	}

	Status RendererImpl::commandList(
		std::uint32_t frameIndex,
		Resource<ICommandList>* commandList)
	{
		if (commandList == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (frameIndex >= CommandLists.size())
		{
			return ERR_INVALID_RANGE;
		}

		if (CommandLists[frameIndex])
		{
			*commandList = CommandLists[frameIndex];
			return {};
		}

		Status status = Device->createCommandList(&CommandLists[frameIndex]);

		if (status == SUCCESS)
		{
			*commandList = CommandLists[frameIndex];
		}

		return status;
	}
} // namespace spall
