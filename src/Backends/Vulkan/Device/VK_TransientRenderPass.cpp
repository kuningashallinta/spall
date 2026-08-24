// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <src/Backends/Vulkan/Common/VK_EnumMappings.h>

#include <optional>
#include <vector>

namespace spall::vk
{
	Status Device::buildRenderPass(
		const Format* colorFormats,
		std::uint32_t colorAttachmentCount,
		Format depthFormat,
		std::uint32_t sampleCount,
		const ColorAttachmentInfo* colorAttachments,
		const DepthStencilAttachmentInfo* depthAttachment,
		VkRenderPass& renderPass) const
	{
		const bool multisampled = (sampleCount > 1);
		const VkSampleCountFlagBits samples = static_cast<VkSampleCountFlagBits>(sampleCount);

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorReferences;
		std::vector<VkAttachmentReference> resolveReferences;
		attachments.reserve((colorAttachmentCount * (multisampled ? 2u : 1u)) + 1u);
		colorReferences.reserve(colorAttachmentCount);

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorAttachmentCount; ++attachmentIndex)
		{
			const std::optional<VkFormat> vkColorFormat = toVkFormat(colorFormats[attachmentIndex]);

			if (not vkColorFormat.has_value())
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			VkAttachmentDescription colorAttachmentDescription = {};
			colorAttachmentDescription.format = vkColorFormat.value();
			colorAttachmentDescription.samples = samples;
			colorAttachmentDescription.loadOp = (colorAttachments != nullptr)
				? attachmentLoadOp(colorAttachments[attachmentIndex].LoadAction)
				: VK_ATTACHMENT_LOAD_OP_LOAD;
			colorAttachmentDescription.storeOp = (colorAttachments != nullptr)
				? attachmentStoreOp(colorAttachments[attachmentIndex].StoreAction)
				: VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments.push_back(colorAttachmentDescription);

			VkAttachmentReference colorReference = {};
			colorReference.attachment = attachmentIndex;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorReferences.push_back(colorReference);
		}

		VkAttachmentReference depthReference = {};
		const VkAttachmentReference* depthReferencePointer = nullptr;

		if (depthFormat != Format::Unknown)
		{
			const std::optional<VkFormat> vkDepthFormat = toVkFormat(depthFormat);

			if (not vkDepthFormat.has_value())
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			VkAttachmentDescription depthAttachmentDescription = {};
			depthAttachmentDescription.format = vkDepthFormat.value();
			depthAttachmentDescription.samples = samples;
			depthAttachmentDescription.loadOp = (depthAttachment != nullptr)
				? attachmentLoadOp(depthAttachment->DepthLoadAction)
				: VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachmentDescription.storeOp = (depthAttachment != nullptr)
				? attachmentStoreOp(depthAttachment->DepthStoreAction)
				: VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachmentDescription.stencilLoadOp = (depthAttachment != nullptr)
				? attachmentLoadOp(depthAttachment->StencilLoadAction)
				: VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachmentDescription.stencilStoreOp = (depthAttachment != nullptr)
				? attachmentStoreOp(depthAttachment->StencilStoreAction)
				: VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments.push_back(depthAttachmentDescription);

			depthReference.attachment = colorAttachmentCount;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthReferencePointer = &depthReference;
		}

		if (multisampled)
		{
			resolveReferences.reserve(colorAttachmentCount);

			for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorAttachmentCount; ++attachmentIndex)
			{
				VkAttachmentDescription resolveDescription = attachments[attachmentIndex];
				resolveDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				resolveDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				resolveDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

				VkAttachmentReference resolveReference = {};
				resolveReference.attachment = static_cast<std::uint32_t>(attachments.size());
				resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				resolveReferences.push_back(resolveReference);
				attachments.push_back(resolveDescription);
			}
		}

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = colorAttachmentCount;
		subpass.pColorAttachments = colorReferences.empty() ? nullptr : colorReferences.data();
		subpass.pResolveAttachments = resolveReferences.empty() ? nullptr : resolveReferences.data();
		subpass.pDepthStencilAttachment = depthReferencePointer;

		const VkSubpassDependency dependency = graphicsSubpassDependency();

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		const VkResult vkResult = vkCreateRenderPass(m_Device, &createInfo, nullptr, &renderPass);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		return {};
	}

	Status Device::createTransientRenderPass(
		const ColorAttachmentInfo* colorAttachments,
		std::uint32_t colorAttachmentCount,
		const DepthStencilAttachmentInfo* depthAttachment,
		const Format* colorFormats,
		Format depthFormat,
		std::uint32_t sampleCount,
		VkRenderPass& renderPass) const
	{
		return buildRenderPass(
			colorFormats,
			colorAttachmentCount,
			(depthAttachment != nullptr) ? depthFormat : Format::Unknown,
			sampleCount,
			colorAttachments,
			depthAttachment,
			renderPass);
	}

	Status Device::createTransientFramebuffer(
		VkRenderPass renderPass,
		const VkImageView* attachments,
		std::uint32_t attachmentCount,
		std::uint32_t width,
		std::uint32_t height,
		VkFramebuffer& framebuffer) const
	{
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = attachmentCount;
		createInfo.pAttachments = attachments;
		createInfo.width = width;
		createInfo.height = height;
		createInfo.layers = 1;

		const VkResult vkResult = vkCreateFramebuffer(m_Device, &createInfo, nullptr, &framebuffer);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		return {};
	}
} // namespace spall::vk
