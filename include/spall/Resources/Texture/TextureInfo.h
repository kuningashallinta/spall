// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#include <cstdint>

namespace spall
{
	struct TextureInfo
	{
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
		std::uint32_t Depth = 1;

		std::uint32_t MipLevels = 1;
		std::uint32_t ArrayLayers = 1;
		std::uint32_t SampleCount = 1;

		bool Cubemap = false;

		spall::Format Format = spall::Format::Unknown;

		TextureUsageFlags Usage = TextureUsageFlags::None;

		ResourceStateFlags InitialState = ResourceStateFlags::Common;
		bool KeepInitialState = false;

		const char* DebugName = nullptr;
	};
} // namespace spall
