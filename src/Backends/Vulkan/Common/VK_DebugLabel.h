// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Color/Color.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

namespace spall::vk
{
	inline VkDebugUtilsLabelEXT debugUtilsLabel(
		const char* label,
		Color color);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_DebugLabel.inl>
