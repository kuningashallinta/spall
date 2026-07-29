#pragma once

#include <spall/Common/Status/Status.h>

#include <dxgi.h>
#include <winerror.h>

namespace spall::dxgi
{
	inline Status mapHResult(HRESULT hr);
} // namespace spall::dxgi

#include <src/Common/DXGI/DXGIError.inl>
