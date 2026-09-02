// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#include <cstdint>

namespace spall
{
	/// Describes a one-dimensional texture to be created by a resource factory.
	struct Texture1DCreateInfo
	{
		std::uint32_t Width = 0;

		/// Number of mip levels to allocate.
		///
		/// Levels beyond the first are created without initial contents.
		std::uint32_t MipLevels = 1;
		std::uint32_t ArrayLayers = 1;

		spall::Format Format = spall::Format::Unknown;
		TextureUsageFlags Usage = TextureUsageFlags::None;

		/// State reported by the texture immediately after creation.
		///
		/// The backend establishes this state when the texture is first used.
		ResourceStateFlags InitialState = ResourceStateFlags::Common;

		/// Restores InitialState after an automatic operation.
		bool KeepInitialState = false;

		const char* DebugName = nullptr;
	};
} // namespace spall
