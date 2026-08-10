// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Pipeline/Pipeline/BlendStateInfo.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <cstdint>
#include <optional>
#include <src/Backends/Vulkan/Common/VK_TextureFormatProperties.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>
#include <src/Backends/Vulkan/Common/VK_VertexFormatProperties.h>
#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline VkPrimitiveTopology vulkanPrimitiveTopology(
		PrimitiveTopology topology);

	inline VkCullModeFlags vulkanCullMode(
		CullMode cullMode);

	inline VkPolygonMode vulkanPolygonMode(
		FillMode fillMode);

	inline VkFrontFace vulkanFrontFace(
		FrontFace frontFace);

	inline VkBlendFactor vulkanBlendFactor(
		BlendFactor blendFactor);

	inline VkCompareOp vulkanCompareOp(
		CompareOp compareOp);

	inline VkStencilOp vulkanStencilOp(
		StencilOp stencilOp);

	inline VkColorComponentFlags vulkanColorComponentFlags(
		ColorComponentFlags mask);

	inline VkBlendOp vulkanBlendOp(
		BlendOp blendOp);

	inline VkPipelineColorBlendAttachmentState vulkanColorBlendAttachmentState(
		const BlendStateInfo& blendState);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_PipelineMappings.inl>
