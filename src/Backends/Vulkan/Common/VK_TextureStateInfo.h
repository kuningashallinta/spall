#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	struct TextureStateInfo
	{
		VkImageLayout layout;
		VkAccessFlags access;
		VkPipelineStageFlags stage;
	};
} // namespace spall::vk
