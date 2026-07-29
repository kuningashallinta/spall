#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChainGeneration.h>

#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/SwapChain/VK_SurfaceLifetime.h>

#include <utility>

namespace spall::vk
{
	SwapChainGeneration::SwapChainGeneration(
		std::shared_ptr<SurfaceLifetime> surface,
		VkSwapchainKHR swapChain)
		: m_Surface(std::move(surface)), m_SwapChain(swapChain)
	{
		SPALL_ASSERT(m_Surface);
		SPALL_ASSERT(m_SwapChain != VK_NULL_HANDLE);
	}

	SwapChainGeneration::~SwapChainGeneration()
	{
		if (m_Surface and m_Surface->m_Device and (m_Surface->m_Device->m_Device != VK_NULL_HANDLE) and (m_SwapChain != VK_NULL_HANDLE))
		{
			vkDestroySwapchainKHR(m_Surface->m_Device->m_Device, m_SwapChain, nullptr);
		}
	}
} // namespace spall::vk
