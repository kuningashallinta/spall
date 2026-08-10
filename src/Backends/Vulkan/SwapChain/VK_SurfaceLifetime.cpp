// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/SwapChain/VK_SurfaceLifetime.h>

#include <src/Backends/Vulkan/Device/VK_Device.h>

namespace spall::vk
{
	SurfaceLifetime::SurfaceLifetime(
		Device& device,
		VkSurfaceKHR surface)
		: m_Device(&device), m_Surface(surface)
	{
		SPALL_ASSERT(m_Surface != VK_NULL_HANDLE);
	}

	SurfaceLifetime::~SurfaceLifetime()
	{
		if (m_Device and (m_Device->m_Instance != VK_NULL_HANDLE) and (m_Surface != VK_NULL_HANDLE))
		{
			vkDestroySurfaceKHR(m_Device->m_Instance, m_Surface, nullptr);
		}
	}
} // namespace spall::vk
