// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>

namespace spall::d3d12
{
	/// Widens an ASCII entry-point name for a DXIL export rename.
	inline std::wstring widenExportName(
		const char* name)
	{
		std::wstring widened;

		for (const char* character = name; *character != '\0'; ++character)
		{
			widened.push_back(static_cast<wchar_t>(*character));
		}

		return widened;
	}

	inline std::wstring generatedExportName(
		const wchar_t* prefix,
		std::uint32_t index)
	{
		return std::wstring(prefix) + std::to_wstring(index);
	}
} // namespace spall::d3d12
