// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Resource<IFrame> IGraphicsQueue::acquireFrame(
		ISwapChain& swapChain)
	{
		Resource<IFrame> frame;
		acquireFrame(swapChain, &frame);
		return frame;
	}
} // namespace spall
