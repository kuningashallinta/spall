// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/TextureUsageFlags.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	inline D3D12_RESOURCE_FLAGS textureUsageFlags(
		TextureUsageFlags usage);

	inline D3D12_FORMAT_SUPPORT1 requiredFormatSupport(
		TextureUsageFlags usage);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_TextureUsageMappings.inl>
