#pragma once

#include <spall/Common/Resource/Resource.h>

#include <src/Backends/Vulkan/Common/VK_Error.h>

namespace spall::vk
{
	class Device;
	class SwapChain;
	class SwapChainGeneration;

	// Outlives individual swap-chain generations so each can safely destroy its native handle first.
	class SurfaceLifetime final
	{
	public:
		SurfaceLifetime(
			Device& device,
			VkSurfaceKHR surface);

		SurfaceLifetime(const SurfaceLifetime&) = delete;
		SurfaceLifetime(SurfaceLifetime&&) = delete;
		~SurfaceLifetime(void);

		SurfaceLifetime& operator=(const SurfaceLifetime&) = delete;
		SurfaceLifetime& operator=(SurfaceLifetime&&) = delete;

	private:
		Resource<Device> m_Device;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

	private:
		friend class SwapChain;
		friend class SwapChainGeneration;
	};
} // namespace spall::vk
