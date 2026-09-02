// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Resources/Texture/TextureInfo.h>

namespace spall
{
	/// Represents a GPU texture of any dimension.
	///
	/// Every texture is an ITexture1D, ITexture2D, or ITexture3D. Consumers that
	/// do not depend on the dimension work through this interface and info().
	class ITexture : public IResource
	{
	public:
		virtual TextureType type(void) const = 0;
		virtual TextureInfo info(void) const = 0;
	};
} // namespace spall
