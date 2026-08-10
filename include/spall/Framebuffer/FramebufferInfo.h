// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Limits.h>

#include <cstdint>

namespace spall
{
	struct FramebufferInfo
	{
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;

		Format ColorFormats[MaxColorAttachments] = {};
		std::uint32_t ColorFormatCount = 0;

		Format DepthFormat = Format::Unknown;
		std::uint32_t SampleCount = 1;
	};
} // namespace spall
