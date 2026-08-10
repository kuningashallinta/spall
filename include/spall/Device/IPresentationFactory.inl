// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

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
