// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/TextureType.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	inline D3D12_RESOURCE_DIMENSION textureType(TextureType type);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_TextureTypeMappings.inl>
