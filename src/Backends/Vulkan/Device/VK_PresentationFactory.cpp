#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <src/Backends/Vulkan/Queue/VK_GraphicsQueue.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChain.h>
#include <src/Validation/Common.h>

#include <windows.h>

#include <cstdint>
#include <memory>

namespace spall::vk
{
	Status Device::createSwapChain(
		const SwapChainCreateInfo& info,
		Resource<ISwapChain>* swapChain)
	{
		if (swapChain == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateSwapChainCreateInfo(info));

		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = GetModuleHandleW(nullptr);
		surfaceCreateInfo.hwnd = static_cast<HWND>(info.Window.Value);

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkResult vkResult = vkCreateWin32SurfaceKHR(m_Instance, &surfaceCreateInfo, nullptr, &surface);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		VkBool32 supportsPresentation = VK_FALSE;
		vkResult = vkGetPhysicalDeviceSurfaceSupportKHR(
			m_PhysicalDevice,
			m_GraphicsQueue->m_FamilyIndex,
			surface,
			&supportsPresentation);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroySurfaceKHR(m_Instance, surface, nullptr);
			return mapVulkanStatus(vkResult);
		}

		if (supportsPresentation == VK_FALSE)
		{
			vkDestroySurfaceKHR(m_Instance, surface, nullptr);
			return ERR_UNSUPPORTED_USAGE;
		}

		std::unique_ptr<SwapChain> vkSwapChain = std::make_unique<SwapChain>(
			*this,
			surface,
			info.Width,
			info.Height,
			info.Format,
			info.PresentMode);

		SPALL_TRY(vkSwapChain->recreate(info.Width, info.Height));

		*swapChain = Resource<ISwapChain>(vkSwapChain.release());

		return {};
	}
} // namespace spall::vk
