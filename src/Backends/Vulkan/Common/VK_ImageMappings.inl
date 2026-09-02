// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline VkImageUsageFlags textureUsageFlags(
		TextureUsageFlags usage)
	{
		VkImageUsageFlags imageUsage = 0;

		if ((usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None)
		{
			imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		if ((usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)
		{
			imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		if ((usage & TextureUsageFlags::TransferSource) != TextureUsageFlags::None)
		{
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if ((usage & TextureUsageFlags::TransferDestination) != TextureUsageFlags::None)
		{
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		if ((usage & TextureUsageFlags::Sampled) != TextureUsageFlags::None)
		{
			imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if ((usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
		{
			imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		return imageUsage;
	}

	inline VkImageType textureType(
		TextureType type)
	{
		switch (type)
		{
			case TextureType::Texture1D:
			{
				return VK_IMAGE_TYPE_1D;
			}

			case TextureType::Texture2D:
			{
				return VK_IMAGE_TYPE_2D;
			}

			case TextureType::Texture3D:
			{
				return VK_IMAGE_TYPE_3D;
			}
		}

		return VK_IMAGE_TYPE_2D;
	}

	inline VkImageViewType textureViewType(
		TextureType type,
		std::uint32_t arrayLayers,
		bool cubemap)
	{
		switch (type)
		{
			case TextureType::Texture1D:
			{
				return (arrayLayers > 1) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
			}

			case TextureType::Texture2D:
			{
				if (cubemap)
				{
					return (arrayLayers > 6) ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
				}

				return (arrayLayers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			}

			case TextureType::Texture3D:
			{
				return VK_IMAGE_VIEW_TYPE_3D;
			}
		}

		return VK_IMAGE_VIEW_TYPE_2D;
	}

	inline std::optional<TextureStateInfo> textureState(
		ResourceStateFlags state)
	{
		switch (state)
		{
			case ResourceStateFlags::RenderTarget:
			{
				return TextureStateInfo {
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
			}

			case ResourceStateFlags::DepthWrite:
			{
				return TextureStateInfo {
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT};
			}

			case ResourceStateFlags::DepthRead:
			{
				return TextureStateInfo {
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT};
			}

			case ResourceStateFlags::ShaderResource:
			{
				return TextureStateInfo {
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
			}

			case ResourceStateFlags::UnorderedAccess:
			{
				return TextureStateInfo {
					VK_IMAGE_LAYOUT_GENERAL,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};
			}

			case ResourceStateFlags::CopySource:
			{
				return TextureStateInfo {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
			}

			case ResourceStateFlags::CopyDest:
			{
				return TextureStateInfo {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
			}

			case ResourceStateFlags::Present:
			{
				return TextureStateInfo {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
			}

			case ResourceStateFlags::Common:
			{
				return TextureStateInfo {
					VK_IMAGE_LAYOUT_GENERAL,
					VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
			}

			case ResourceStateFlags::Unknown:
			{
				return TextureStateInfo {VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
			}

			default:
			{
				return std::nullopt;
			}
		}
	}
} // namespace spall::vk
