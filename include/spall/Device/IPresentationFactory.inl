namespace spall
{
	inline Resource<ISwapChain> IPresentationFactory::createSwapChain(
		const SwapChainCreateInfo& info)
	{
		Resource<ISwapChain> swapChain;
		createSwapChain(info, &swapChain);
		return swapChain;
	}
} // namespace spall
