// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

namespace spall
{
	/// Describes depth and stencil attachment operations for a render pass.
	struct DepthStencilAttachmentInfo
	{
		LoadAction DepthLoadAction = LoadAction::Load;
		StoreAction DepthStoreAction = StoreAction::Store;

		/// Used only when DepthLoadAction is Clear.
		float ClearDepth = 1.0f;

		LoadAction StencilLoadAction = LoadAction::Load;
		StoreAction StencilStoreAction = StoreAction::Store;

		/// Used only when StencilLoadAction is Clear.
		std::uint8_t ClearStencil = 0;
	};
} // namespace spall
