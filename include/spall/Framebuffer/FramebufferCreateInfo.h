// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>

#include <cstdint>

namespace spall
{
	class ITextureView;

	/// Describes the attachment views grouped by a framebuffer.
	///
	/// Every attachment must have the same extent and expose exactly one mip level.
	struct FramebufferCreateInfo
	{
		ITextureView* ColorAttachments[MaxColorAttachments] = {};
		std::uint32_t ColorAttachmentCount = 0;
		ITextureView* DepthAttachment = nullptr;

		/// Single-sampled targets receiving each multisampled color attachment.
		///
		/// Required when the color attachments are multisampled, unused otherwise.
		ITextureView* ResolveAttachments[MaxColorAttachments] = {};
	};
} // namespace spall
