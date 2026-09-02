// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Resources/Texture/ITexture.h>

#include <cstdint>

namespace spall
{
	/// Represents a two-dimensional GPU texture.
	///
	/// A cubemap is a two-dimensional texture whose array layers are addressed
	/// as cube faces in groups of six.
	class ITexture2D : public ITexture
	{
	public:
		virtual std::uint32_t width(void) const = 0;
		virtual std::uint32_t height(void) const = 0;
		virtual std::uint32_t arrayLayers(void) const = 0;
		virtual std::uint32_t sampleCount(void) const = 0;
		virtual bool isCubemap(void) const = 0;
	};
} // namespace spall
