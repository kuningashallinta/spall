// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>
#include <spall/Framebuffer/FramebufferInfo.h>

namespace spall
{
	/// Groups render-pass attachment views with a common extent.
	class IFramebuffer : public IResource
	{
	public:
		virtual FramebufferInfo info(void) const = 0;
	};
} // namespace spall
