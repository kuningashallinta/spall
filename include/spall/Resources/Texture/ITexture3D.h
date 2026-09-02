// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Resources/Texture/ITexture.h>

#include <cstdint>

namespace spall
{
	/// Represents a three-dimensional GPU texture.
	class ITexture3D : public ITexture
	{
	public:
		virtual std::uint32_t width(void) const = 0;
		virtual std::uint32_t height(void) const = 0;
		virtual std::uint32_t depth(void) const = 0;
	};
} // namespace spall
