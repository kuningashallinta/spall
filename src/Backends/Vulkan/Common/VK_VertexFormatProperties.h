// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace spall::vk
{
	struct VertexFormatProperties
	{
		VkFormat format = VK_FORMAT_UNDEFINED;
		std::uint32_t size = 0;
	};
} // namespace spall::vk
