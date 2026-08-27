// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#include <cstdint>
#include <optional>
#include <src/Backends/Vulkan/Common/VK_FormatMappings.h>
#include <src/Backends/Vulkan/Common/VK_TextureFormatProperties.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>
#include <src/Backends/Vulkan/Common/VK_VertexFormatProperties.h>
#include <src/Validation/Common/FormatValidation.h>
#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline VkIndexType indexFormat(IndexFormat format);

	inline std::optional<VertexFormatProperties> vertexFormatInfo(Format format);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_VertexInputMappings.inl>
