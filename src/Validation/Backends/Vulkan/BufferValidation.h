// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Resources/Buffer/BufferCreateInfo.h>

#include <vulkan/vulkan.h>

namespace spall::vk
{
	class BufferValidation
	{
	public:
		static Status validateCreateInfo(
			const BufferCreateInfo& info,
			const VkPhysicalDeviceLimits& limits);
	};
} // namespace spall::vk
