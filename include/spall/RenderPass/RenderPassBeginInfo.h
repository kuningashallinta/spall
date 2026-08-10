// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/RenderPass/ColorAttachmentInfo.h>
#include <spall/RenderPass/DepthStencilAttachmentInfo.h>

namespace spall
{
	class IFramebuffer;

	/// Describes one render pass and the attachment operations performed by it.
	struct RenderPassBeginInfo
	{
		IFramebuffer* Framebuffer = nullptr;

		/// Operations correspond positionally to the framebuffer color attachments.
		ColorAttachmentInfo ColorAttachments[MaxColorAttachments] = {};
		DepthStencilAttachmentInfo DepthAttachment = {};
	};
} // namespace spall
