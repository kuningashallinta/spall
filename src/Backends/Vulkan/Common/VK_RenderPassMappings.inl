// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline VkAttachmentLoadOp attachmentLoadOp(
		LoadAction loadOp)
	{
		switch (loadOp)
		{
			case LoadAction::Clear:
			{
				return VK_ATTACHMENT_LOAD_OP_CLEAR;
			}

			case LoadAction::DontCare:
			{
				return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}

			case LoadAction::Load:
			default:
			{
				return VK_ATTACHMENT_LOAD_OP_LOAD;
			}
		}
	}

	inline VkAttachmentStoreOp attachmentStoreOp(
		StoreAction storeOp)
	{
		switch (storeOp)
		{
			case StoreAction::DontCare:
			{
				return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}

			case StoreAction::Store:
			default:
			{
				return VK_ATTACHMENT_STORE_OP_STORE;
			}
		}
	}

	inline VkSubpassDependency graphicsSubpassDependency(
		void)
	{
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = dependency.srcStageMask;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		return dependency;
	}
} // namespace spall::vk
