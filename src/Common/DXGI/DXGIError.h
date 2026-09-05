// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>

#include <dxgi.h>
#include <winerror.h>

namespace spall::d3d12
{
	inline Status mapStatus(HRESULT hr);
} // namespace spall::d3d12

#include <src/Common/DXGI/DXGIError.inl>
