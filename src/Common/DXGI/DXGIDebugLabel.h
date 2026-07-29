#pragma once

#include <spall/Common/Status/Status.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <windows.h>

namespace spall::dxgi
{
	/// Marker payload versions the D3D12 event API accepts; a wide string is the widest understood form.
	inline constexpr UINT UnicodeEventVersion = 0;
	inline constexpr UINT AnsiEventVersion = 1;

	inline Status wideDebugLabel(
		const char* label,
		std::wstring* wideLabel);

	/// Gets the byte size an event payload needs, which covers the terminator the decoder looks for.
	inline UINT eventPayloadSize(
		const std::wstring& wideLabel)
	{
		return static_cast<UINT>((wideLabel.size() + 1) * sizeof(wchar_t));
	}
} // namespace spall::dxgi

#include <src/Common/DXGI/DXGIDebugLabel.inl>
