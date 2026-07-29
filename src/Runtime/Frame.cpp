#include <spall/Runtime/Frame.h>

#include <spall/Common/Assert.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <src/Runtime/RendererImpl.h>

#include <cstdint>
#include <utility>

namespace spall
{
	Frame::Frame(void) = default;

	Frame::Frame(
		Frame&& other) noexcept
		: m_Renderer(std::exchange(other.m_Renderer, nullptr)), m_Frame(std::move(other.m_Frame)), m_Framebuffer(std::move(other.m_Framebuffer)), m_CommandList(std::move(other.m_CommandList))
	{
	}

	Frame::~Frame(
		void)
	{
		cancel();
	}

	Frame& Frame::operator=(
		Frame&& other) noexcept
	{
		if (this != &other)
		{
			cancel();

			m_Renderer = std::exchange(other.m_Renderer, nullptr);
			m_Frame = std::move(other.m_Frame);
			m_Framebuffer = std::move(other.m_Framebuffer);
			m_CommandList = std::move(other.m_CommandList);
		}

		return *this;
	}

	Frame::operator bool(
		void) const noexcept
	{
		return m_Renderer != nullptr;
	}

	IFramebuffer& Frame::framebuffer(
		void) const
	{
		SPALL_ASSERT(m_Renderer != nullptr);
		return *m_Framebuffer;
	}

	ICommandList& Frame::commands(
		void) const
	{
		SPALL_ASSERT(m_Renderer != nullptr);
		return *m_CommandList;
	}

	Status Frame::end(
		void)
	{
		if (m_Renderer == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		Status status = m_CommandList->endRenderPass();

		if (status == SUCCESS)
		{
			status = m_CommandList->end();
		}

		if (status == SUCCESS)
		{
			status = m_Renderer->Device->graphicsQueue().submit(*m_CommandList);
		}

		if (status == SUCCESS)
		{
			status = m_Renderer->Device->graphicsQueue().present(*m_Frame);
		}

		m_Renderer->FrameActive = false;
		m_Renderer = nullptr;
		m_CommandList.reset();
		m_Framebuffer.reset();
		m_Frame.reset();

		return status;
	}

	Frame::Frame(
		RendererImpl& renderer,
		Resource<IFrame> frame,
		Resource<IFramebuffer> framebuffer,
		Resource<ICommandList> commandList)
		: m_Renderer(&renderer), m_Frame(std::move(frame)), m_Framebuffer(std::move(framebuffer)), m_CommandList(std::move(commandList))
	{
	}

	void Frame::cancel(
		void)
	{
		if (m_Renderer == nullptr)
		{
			return;
		}

		const std::uint32_t frameIndex = m_Frame->index();

		if (frameIndex < m_Renderer->CommandLists.size())
		{
			m_Renderer->CommandLists[frameIndex].reset();
		}

		for (Resource<IFramebuffer>& framebuffer : m_Renderer->Framebuffers)
		{
			framebuffer.reset();
		}

		m_Renderer->FrameActive = false;
		m_Renderer = nullptr;
	}
} // namespace spall
