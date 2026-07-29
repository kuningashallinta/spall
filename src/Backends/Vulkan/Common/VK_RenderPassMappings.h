#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <cstdint>
#include <optional>
#include <src/Backends/Vulkan/Common/VK_TextureFormatProperties.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>
#include <src/Backends/Vulkan/Common/VK_VertexFormatProperties.h>
#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline VkAttachmentLoadOp attachmentLoadOp(
		LoadAction loadOp);

	inline VkAttachmentStoreOp attachmentStoreOp(
		StoreAction storeOp);

	inline VkSubpassDependency graphicsSubpassDependency(
		void);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_RenderPassMappings.inl>
