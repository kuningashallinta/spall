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
