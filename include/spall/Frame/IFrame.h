// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <cstdint>

namespace spall
{
	class ITexture;
	class ITextureView;

	/// Represents one acquired swap-chain image.
	/// Releasing an unpresented frame cancels the queue's active presentation cycle.
	class IFrame : public IResource
	{
	public:
		virtual std::uint32_t index(void) const = 0;
		virtual ITexture& presentTexture(void) = 0;
		virtual ITextureView& presentTextureView(void) = 0;
	};
} // namespace spall
