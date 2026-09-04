// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Common/DXGI/DXGIDebugLabel.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <string>

namespace spall::d3d12
{
	/// Forwards a UTF-8 debug name to the object, where a graphics debugger reads it back.
	///
	/// A null or empty name is a no-op. A name that is not valid UTF-8 reports
	/// ERR_INVALID_ARGUMENT.
	inline Status setObjectName(
		ID3D12Object& object,
		const char* name);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/D3D12_ObjectName.inl>
