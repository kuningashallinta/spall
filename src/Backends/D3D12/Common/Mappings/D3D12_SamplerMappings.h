#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	inline D3D12_FILTER samplerFilter(
		Filter minFilter,
		Filter magFilter,
		Filter mipFilter,
		bool anisotropic,
		bool comparisonEnabled);

	inline D3D12_TEXTURE_ADDRESS_MODE addressMode(
		AddressMode mode);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_SamplerMappings.inl>
