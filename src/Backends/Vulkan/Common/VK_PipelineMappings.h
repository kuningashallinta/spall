// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Pipeline/Pipeline/BlendStateInfo.h>

#include <cstdint>
#include <optional>
#include <src/Backends/Vulkan/Common/VK_TextureFormatProperties.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>
#include <src/Backends/Vulkan/Common/VK_VertexFormatProperties.h>
#include <vulkan/vulkan.h>

namespace spall::vk
{
	inline VkPrimitiveTopology primitiveTopology(PrimitiveTopology topology);

	inline VkCullModeFlags cullMode(CullMode mode);

	inline VkPolygonMode fillMode(FillMode mode);

	inline VkFrontFace frontFace(FrontFace face);

	inline VkBlendFactor blendFactor(BlendFactor factor);

	inline VkCompareOp compareOp(CompareOp op);

	inline VkStencilOp stencilOp(StencilOp op);

	inline VkColorComponentFlags colorComponentFlags(ColorComponentFlags mask);

	inline VkBlendOp blendOp(BlendOp op);

	inline VkPipelineColorBlendAttachmentState blendState(const BlendStateInfo& info);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_PipelineMappings.inl>
