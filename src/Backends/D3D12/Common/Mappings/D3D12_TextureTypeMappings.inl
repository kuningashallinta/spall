// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::d3d12
{
	inline D3D12_RESOURCE_DIMENSION textureType(
		TextureType type)
	{
		switch (type)
		{
			case TextureType::Texture1D:
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			}

			case TextureType::Texture2D:
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			}

			case TextureType::Texture3D:
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
		}

		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}
} // namespace spall::d3d12
