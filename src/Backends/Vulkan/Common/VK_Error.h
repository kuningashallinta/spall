// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline Status mapVulkanStatus(VkResult result);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_Error.inl>
