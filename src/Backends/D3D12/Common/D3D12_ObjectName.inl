// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::d3d12
{
	inline Status setObjectName(
		ID3D12Object& object,
		const char* name)
	{
		if ((name == nullptr) or (name[0] == '\0'))
		{
			return {};
		}

		std::wstring wideName;

		SPALL_TRY(wideDebugLabel(name, &wideName));

		return mapStatus(object.SetName(wideName.c_str()));
	}
} // namespace spall::d3d12
