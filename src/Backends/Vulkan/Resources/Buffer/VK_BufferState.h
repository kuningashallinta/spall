// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Resources/Buffer/BufferInfo.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <cstdint>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	struct BufferStateInfo
	{
		VkAccessFlags access = 0;
		VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	};

	inline std::optional<BufferStateInfo> vulkanBufferState(
		ResourceStateFlags state);
} // namespace spall::vk

#include <src/Backends/Vulkan/Resources/Buffer/VK_BufferState.inl>
