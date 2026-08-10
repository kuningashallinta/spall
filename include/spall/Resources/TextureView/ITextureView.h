// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

namespace spall
{
	class ITexture;

	/// Selects the aspects and mip range exposed from a texture.
	class ITextureView : public IResource
	{
	public:
		virtual ITexture& texture(void) const = 0;
		virtual Format format(void) const = 0;
		virtual TextureAspectFlags aspects(void) const = 0;
		virtual std::uint32_t baseMipLevel(void) const = 0;
		virtual std::uint32_t mipLevels(void) const = 0;
		virtual std::uint32_t baseArrayLayer(void) const = 0;
		virtual std::uint32_t arrayLayers(void) const = 0;
		virtual bool isCubemap(void) const = 0;
	};
} // namespace spall
