// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vulkan/vulkan.h>

namespace spall::vk
{
	struct TextureStateInfo
	{
		VkImageLayout layout;
		VkAccessFlags access;
		VkPipelineStageFlags stage;
	};
} // namespace spall::vk
