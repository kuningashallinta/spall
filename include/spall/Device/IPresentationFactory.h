#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/SwapChain/ISwapChain.h>
#include <spall/SwapChain/SwapChainCreateInfo.h>

namespace spall
{
	/// Creates presentation resources owned by a graphics device.
	///
	/// The convenience overload returns an empty resource on failure; the Status
	/// overload preserves detailed failure information.
	class IPresentationFactory
	{
	public:
		virtual ~IPresentationFactory(void) = default;

		Resource<ISwapChain> createSwapChain(
			const SwapChainCreateInfo& info);

		virtual Status createSwapChain(
			const SwapChainCreateInfo& info,
			Resource<ISwapChain>* swapChain) = 0;
	};
} // namespace spall

#include <spall/Device/IPresentationFactory.inl>
