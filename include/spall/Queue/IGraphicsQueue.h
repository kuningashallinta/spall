#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Frame/IFrame.h>
#include <spall/Queue/IQueue.h>

namespace spall
{
	class ISwapChain;

	/// Owns command submission and the acquire-submit-present sequence for one device.
	class IGraphicsQueue : public IQueue
	{
	public:
		/// Acquires a frame, returning an empty resource on failure.
		Resource<IFrame> acquireFrame(
			ISwapChain& swapChain);

		/// Begins a presentation cycle. Only one acquired frame may be active on the queue.
		virtual Status acquireFrame(
			ISwapChain& swapChain,
			Resource<IFrame>* frame) = 0;

		/// Presents the active frame after command-list work targeting it has been submitted.
		virtual Status present(IFrame& frame) = 0;
	};
} // namespace spall

#include <spall/Queue/IGraphicsQueue.inl>
