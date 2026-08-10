#pragma once

#include <spall/Common/Enums/Format.h>
#include <spall/Common/Enums/IndexFormat.h>

#include <dxgiformat.h>

namespace spall::d3d12
{
	inline DXGI_FORMAT nativeFormat(
		Format format);

	inline DXGI_FORMAT nativeSwapChainFormat(
		Format format);

	inline DXGI_FORMAT nativeIndexFormat(
		IndexFormat format);
} // namespace spall::d3d12

#include <src/Common/DXGI/DXGIFormatMappings.inl>
