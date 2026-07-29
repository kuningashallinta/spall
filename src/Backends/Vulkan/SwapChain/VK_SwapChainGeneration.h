#pragma once

#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <memory>

namespace spall::vk
{
	class GraphicsQueue;
	class SurfaceLifetime;
	class SwapChain;

	// Wrapped images retain their native swap-chain generation so it cannot be destroyed first.
	class SwapChainGeneration final
	{
	public:
		SwapChainGeneration(
			std::shared_ptr<SurfaceLifetime> surface,
			VkSwapchainKHR swapChain);

		SwapChainGeneration(const SwapChainGeneration&) = delete;
		SwapChainGeneration(SwapChainGeneration&&) = delete;
		~SwapChainGeneration(void);

		SwapChainGeneration& operator=(const SwapChainGeneration&) = delete;
		SwapChainGeneration& operator=(SwapChainGeneration&&) = delete;

	private:
		std::shared_ptr<SurfaceLifetime> m_Surface;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

	private:
		friend class GraphicsQueue;
		friend class SwapChain;
	};
} // namespace spall::vk
