#pragma once

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	inline D3D12_RESOURCE_STATES nativeResourceState(
		ResourceStateFlags state);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_ResourceStateMappings.inl>
