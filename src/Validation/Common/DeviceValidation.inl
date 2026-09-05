// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Status validateFormatCapabilityQuery(
		Format format,
		FormatCapabilities* capabilities)
	{
		if (capabilities == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (not isTextureFormat(format) and not isVertexFormat(format))
		{
			return ERR_INVALID_FORMAT;
		}

		return {};
	}
} // namespace spall
