// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#include <cstdint>
#include <optional>
#include <src/Backends/Vulkan/Common/VK_TextureFormatProperties.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>
#include <src/Backends/Vulkan/Common/VK_VertexFormatProperties.h>
#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline VkShaderStageFlags vulkanShaderStageFlags(
		ShaderStageFlags stages);

	inline VkDescriptorType vulkanDescriptorType(
		ResourceBindingType type);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_BindingMappings.inl>
