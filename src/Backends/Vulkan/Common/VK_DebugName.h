#pragma once

#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>

namespace spall::vk
{
	inline void setDebugName(
		VkDevice device,
		VkObjectType objectType,
		std::uint64_t objectHandle,
		const char* name);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_DebugName.inl>
