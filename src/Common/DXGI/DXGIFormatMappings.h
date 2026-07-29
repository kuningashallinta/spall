#pragma once

#include <spall/Common/Enums/Format.h>
#include <spall/Common/Enums/IndexFormat.h>

#include <dxgiformat.h>

namespace spall::dxgi
{
	inline DXGI_FORMAT nativeFormat(
		Format format);

	inline DXGI_FORMAT nativeSwapChainFormat(
		Format format);

	inline DXGI_FORMAT nativeIndexFormat(
		IndexFormat format);
} // namespace spall::dxgi

#include <src/Common/DXGI/DXGIFormatMappings.inl>
