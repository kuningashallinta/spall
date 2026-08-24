// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>

#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline Status mapStatus(VkResult result);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_Error.inl>
