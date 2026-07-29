#include <spall/Runtime/Renderer.h>

#include <spall/Backend/BackendFactory.h>
#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Assert.h>
#include <spall/Device/IDevice.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/RenderPass/RenderPassBeginInfo.h>
#include <spall/SwapChain/ISwapChain.h>
#include <src/Runtime/RendererImpl.h>
#include <src/Validation/Common/FormatValidation.h>

#include <memory>
#include <utility>

namespace spall
{
	namespace
	{
		void destroy(
			std::unique_ptr<RendererImpl>& impl)
		{
			if (impl)
			{
				SPALL_ASSERT(not impl->FrameActive);
				impl->Device->graphicsQueue().waitIdle();
				impl.reset();
			}
		}
	} // namespace

	Renderer::Renderer(void) = default;

	Renderer::Renderer(
		Renderer&& other) noexcept
		: m_Impl(std::move(other.m_Impl))
	{
	}

	Renderer::~Renderer(
		void)
	{
		destroy(m_Impl);
	}

	Renderer& Renderer::operator=(
		Renderer&& other) noexcept
	{
		if (this != &other)
		{
			destroy(m_Impl);
			m_Impl = std::move(other.m_Impl);
		}

		return *this;
	}

	Renderer::operator bool(
		void) const noexcept
	{
		return m_Impl != nullptr;
	}

	Status Renderer::create(
		const RendererCreateInfo& info,
		Renderer* renderer)
	{
		if (renderer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		*renderer = Renderer();

		std::unique_ptr<IBackend> backend;
		Status status = createBackend(info.Backend, &backend);

		if (status != SUCCESS)
		{
			return status;
		}

		Resource<IDevice> device;
		status = backend->createDevice(info.Device, &device);

		if (status != SUCCESS)
		{
			return status;
		}

		Resource<ISwapChain> swapChain;
		status = device->presentation().createSwapChain(info.SwapChain, &swapChain);

		if (status != SUCCESS)
		{
			return status;
		}

		std::unique_ptr<RendererImpl> impl = std::make_unique<RendererImpl>(
			std::move(backend),
			std::move(device),
			std::move(swapChain),
			info.SwapChain.Width,
			info.SwapChain.Height,
			info.DepthStencilFormat);

		status = impl->createDepthAttachment();

		if (status != SUCCESS)
		{
			destroy(impl);
			return status;
		}

		renderer->m_Impl = std::move(impl);
		return {};
	}

	Renderer Renderer::create(
		const RendererCreateInfo& info)
	{
		Renderer renderer;
		create(info, &renderer);
		return renderer;
	}

	RenderBackendType Renderer::backendType(
		void) const
	{
		return m_Impl->Backend->backendType();
	}

	std::uint32_t Renderer::width(
		void) const
	{
		return m_Impl->Width;
	}

	std::uint32_t Renderer::height(
		void) const
	{
		return m_Impl->Height;
	}

	Format Renderer::colorFormat(
		void) const
	{
		return m_Impl->SwapChain->format();
	}

	Format Renderer::depthStencilFormat(
		void) const
	{
		return m_Impl->DepthStencilFormat;
	}

	IDevice& Renderer::device(
		void) const
	{
		return *m_Impl->Device;
	}

	ISwapChain& Renderer::swapChain(
		void) const
	{
		return *m_Impl->SwapChain;
	}

	Status Renderer::beginFrame(
		const FrameBeginInfo& info,
		Frame* frame)
	{
		if (frame == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (*frame)
		{
			return ERR_INVALID_STATE;
		}

		if (m_Impl->FrameActive)
		{
			return ERR_INVALID_STATE;
		}

		if ((m_Impl->Width == 0) or (m_Impl->Height == 0))
		{
			return ERR_INVALID_SIZE;
		}

		if ((m_Impl->DepthStencilFormat != Format::Unknown) and (not m_Impl->DepthView))
		{
			return ERR_INVALID_STATE;
		}

		Resource<IFrame> acquiredFrame;
		Status status = m_Impl->Device->graphicsQueue().acquireFrame(*m_Impl->SwapChain, &acquiredFrame);

		if (status != SUCCESS)
		{
			return status;
		}

		Resource<IFramebuffer> framebuffer;
		status = m_Impl->framebuffer(*acquiredFrame, &framebuffer);

		if (status != SUCCESS)
		{
			return status;
		}

		Resource<ICommandList> commandList;
		status = m_Impl->commandList(acquiredFrame->index(), &commandList);

		if (status != SUCCESS)
		{
			return status;
		}

		status = commandList->begin();

		if (status != SUCCESS)
		{
			return status;
		}

		RenderPassBeginInfo renderPass = {};
		renderPass.Framebuffer = framebuffer.get();
		renderPass.ColorAttachments[0].LoadAction = LoadAction::Clear;
		renderPass.ColorAttachments[0].StoreAction = StoreAction::Store;
		renderPass.ColorAttachments[0].ClearColor = info.ClearColor;

		if (m_Impl->DepthStencilFormat != Format::Unknown)
		{
			renderPass.DepthAttachment.DepthLoadAction = LoadAction::Clear;
			renderPass.DepthAttachment.DepthStoreAction = StoreAction::DontCare;
			renderPass.DepthAttachment.ClearDepth = info.ClearDepth;
			renderPass.DepthAttachment.StencilLoadAction = LoadAction::DontCare;
			renderPass.DepthAttachment.StencilStoreAction = StoreAction::DontCare;

			if (hasStencilAspect(m_Impl->DepthStencilFormat))
			{
				renderPass.DepthAttachment.StencilLoadAction = LoadAction::Clear;
				renderPass.DepthAttachment.ClearStencil = info.ClearStencil;
			}
		}

		status = commandList->beginRenderPass(renderPass);

		if (status != SUCCESS)
		{
			return status;
		}

		Viewport viewport = {};
		viewport.Width = static_cast<float>(m_Impl->Width);
		viewport.Height = static_cast<float>(m_Impl->Height);
		viewport.MaxDepth = 1.0f;
		status = commandList->setViewport(viewport);

		if (status != SUCCESS)
		{
			return status;
		}

		Scissor scissor = {};
		scissor.Width = m_Impl->Width;
		scissor.Height = m_Impl->Height;
		status = commandList->setScissor(scissor);

		if (status != SUCCESS)
		{
			return status;
		}

		m_Impl->FrameActive = true;
		*frame = Frame(
			*m_Impl,
			std::move(acquiredFrame),
			std::move(framebuffer),
			std::move(commandList));

		return {};
	}

	Frame Renderer::beginFrame(
		const FrameBeginInfo& info)
	{
		Frame frame;
		beginFrame(info, &frame);
		return frame;
	}

	Status Renderer::resize(
		std::uint32_t width,
		std::uint32_t height)
	{
		if (m_Impl->FrameActive)
		{
			return ERR_INVALID_STATE;
		}

		Status status = m_Impl->Device->graphicsQueue().waitIdle();

		if (status != SUCCESS)
		{
			return status;
		}

		m_Impl->releaseFramebuffers();

		status = m_Impl->SwapChain->resize(width, height);

		if (status != SUCCESS)
		{
			m_Impl->Width = 0;
			m_Impl->Height = 0;
			return status;
		}

		m_Impl->Width = width;
		m_Impl->Height = height;
		m_Impl->Framebuffers.resize(m_Impl->SwapChain->frameCount());
		m_Impl->CommandLists.resize(m_Impl->SwapChain->frameCount());

		return m_Impl->createDepthAttachment();
	}
} // namespace spall
