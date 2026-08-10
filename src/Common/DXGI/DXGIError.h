#pragma once

#include <spall/Common/Status/Status.h>

#include <dxgi.h>
#include <winerror.h>

namespace spall::d3d12
{
	inline Status mapHResult(HRESULT hr);
} // namespace spall::d3d12

#include <src/Common/DXGI/DXGIError.inl>
