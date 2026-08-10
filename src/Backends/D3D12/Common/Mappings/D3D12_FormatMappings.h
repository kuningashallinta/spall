// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Device/FormatCapabilities.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common/FormatValidation.h>

namespace spall::d3d12
{
	inline FormatCapabilities formatCapabilities(
		Format format,
		D3D12_FORMAT_SUPPORT1 support);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_FormatMappings.inl>
