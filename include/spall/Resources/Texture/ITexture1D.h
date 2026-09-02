// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Resources/Texture/ITexture.h>

#include <cstdint>

namespace spall
{
	/// Represents a one-dimensional GPU texture.
	class ITexture1D : public ITexture
	{
	public:
		virtual std::uint32_t width(void) const = 0;
		virtual std::uint32_t arrayLayers(void) const = 0;
	};
} // namespace spall
