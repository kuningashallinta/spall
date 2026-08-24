// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Resources/Texture/TextureInfo.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	/// Gets the flat subresource index D3D12 uses, which orders mip levels before array layers.
	inline UINT subresourceIndex(
		const TextureInfo& info,
		std::uint32_t mipLevel,
		std::uint32_t arrayLayer)
	{
		return static_cast<UINT>(mipLevel + (arrayLayer * info.MipLevels));
	}
} // namespace spall::d3d12
