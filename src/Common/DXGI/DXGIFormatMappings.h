#pragma once

#include <spall/Common/Enums/Format.h>
#include <spall/Common/Enums/IndexFormat.h>

#include <dxgiformat.h>

namespace spall::d3d12
{
	inline DXGI_FORMAT format(
		Format format);

	inline DXGI_FORMAT swapChainFormat(
		Format format);

	inline DXGI_FORMAT indexFormat(
		IndexFormat format);
} // namespace spall::d3d12

#include <src/Common/DXGI/DXGIFormatMappings.inl>
