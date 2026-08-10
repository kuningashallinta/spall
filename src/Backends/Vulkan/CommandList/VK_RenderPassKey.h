// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/Format.h>
#include <spall/Common/Enums/LoadAction.h>
#include <spall/Common/Enums/StoreAction.h>
#include <spall/Common/Limits.h>

#include <cstdint>

namespace spall::vk
{
	struct RenderPassKey
	{
		Format ColorFormats[MaxColorAttachments] = {};
		LoadAction ColorLoadActions[MaxColorAttachments] = {};
		StoreAction ColorStoreActions[MaxColorAttachments] = {};
		std::uint32_t ColorCount = 0;
		Format DepthFormat = Format::Unknown;
		LoadAction DepthLoadAction = LoadAction::Load;
		StoreAction DepthStoreAction = StoreAction::Store;
		LoadAction StencilLoadAction = LoadAction::Load;
		StoreAction StencilStoreAction = StoreAction::Store;
		std::uint32_t SampleCount = 1;

		bool operator==(const RenderPassKey& other) const = default;
	};
} // namespace spall::vk
