// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Device/FormatCapabilities.h>
#include <src/Validation/Common/FormatValidation.h>

#include <cstdint>
#include <optional>
#include <src/Backends/Vulkan/Common/VK_TextureFormatProperties.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>
#include <src/Backends/Vulkan/Common/VK_VertexFormatProperties.h>
#include <vulkan/vulkan.h>

namespace spall::vk
{
	inline std::optional<VkFormat> toVkFormat(Format format);

	inline std::optional<Format> toSpallFormat(VkFormat format);

	inline FormatCapabilities formatCapabilities(
		Format format,
		const VkFormatProperties& properties);

	inline VkImageAspectFlags aspectMask(TextureAspectFlags aspects);

	inline TextureAspectFlags defaultAspects(Format format);

	inline std::optional<TextureFormatProperties> textureFormatInfo(Format format);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_FormatMappings.inl>
