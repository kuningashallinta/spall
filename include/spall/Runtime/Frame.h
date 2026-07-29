#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>

namespace spall
{
	class ICommandList;
	class IFrame;
	class IFramebuffer;
	class Renderer;
	class RendererImpl;

	/// Represents one managed frame being recorded for presentation.
	///
	/// The frame must not outlive its Renderer. Destroying an unfinished
	/// frame cancels it without submitting or presenting.
	class Frame
	{
	public:
		Frame(void);
		Frame(Frame&& other) noexcept;

		~Frame(void);

		Frame& operator=(Frame&& other) noexcept;

		explicit operator bool(void) const noexcept;

		IFramebuffer& framebuffer(void) const;
		ICommandList& commands(void) const;

		/// Closes the default render pass, then submits and presents the frame.
		Status end(void);

	private:
		Frame(
			RendererImpl& renderer,
			Resource<IFrame> frame,
			Resource<IFramebuffer> framebuffer,
			Resource<ICommandList> commandList);

		/// Drops the open recording and every cached framebuffer, which a cancelled
		/// frame can invalidate by forcing the swap chain to be recreated.
		void cancel(void);

		RendererImpl* m_Renderer = nullptr;
		Resource<IFrame> m_Frame;
		Resource<IFramebuffer> m_Framebuffer;
		Resource<ICommandList> m_CommandList;

		friend class Renderer;
	};
} // namespace spall
