// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	struct VertexFormatProperties
	{
		VkFormat format = VK_FORMAT_UNDEFINED;
		std::uint32_t size = 0;
	};
} // namespace spall::vk
