// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::d3d12
{
	inline Status wideDebugLabel(
		const char* label,
		std::wstring* wideLabel)
	{
		const int characterCount = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, label, -1, nullptr, 0);

		if (characterCount == 0)
		{
			return ERR_INVALID_ARGUMENT;
		}

		std::wstring convertedLabel(static_cast<std::size_t>(characterCount), L'\0');
		const int convertedCount = MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			label,
			-1,
			convertedLabel.data(),
			characterCount);

		if (convertedCount == 0)
		{
			return ERR_INVALID_ARGUMENT;
		}

		convertedLabel.pop_back();
		*wideLabel = std::move(convertedLabel);

		return {};
	}
} // namespace spall::d3d12
