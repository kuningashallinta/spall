// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::d3d12
{
	inline D3D12_FILTER samplerFilter(
		Filter minFilter,
		Filter magFilter,
		Filter mipFilter,
		bool anisotropic,
		bool comparisonEnabled)
	{
		if (anisotropic)
		{
			return comparisonEnabled ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
		}

		const D3D12_FILTER_TYPE min = (minFilter == Filter::Linear) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
		const D3D12_FILTER_TYPE mag = (magFilter == Filter::Linear) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
		const D3D12_FILTER_TYPE mip = (mipFilter == Filter::Linear) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
		const D3D12_FILTER_REDUCTION_TYPE reduction = comparisonEnabled ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;

		return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, reduction);
	}

	inline D3D12_TEXTURE_ADDRESS_MODE addressMode(
		AddressMode mode)
	{
		switch (mode)
		{
			case AddressMode::ClampToEdge:
			{
				return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			}

			case AddressMode::Repeat:
			default:
			{
				return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
		}
	}
} // namespace spall::d3d12
