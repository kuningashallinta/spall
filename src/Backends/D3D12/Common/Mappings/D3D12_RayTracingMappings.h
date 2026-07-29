#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <span>

namespace spall::d3d12
{
	inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE nativeAccelerationStructureType(
		AccelerationStructureType type);

	/// Maps the portable build flags. A recorded update contributes its own
	/// flag, so PERFORM_UPDATE is never produced here.
	inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nativeAccelerationStructureBuildFlags(
		AccelerationStructureBuildFlags flags);

	inline D3D12_RAYTRACING_GEOMETRY_FLAGS nativeAccelerationStructureGeometryFlags(
		AccelerationStructureGeometryFlags flags);

	/// Reports whether a format is legal for a traced vertex position.
	inline bool isSupportedAccelerationStructureVertexFormat(
		Format format);

	/// Assembles the build inputs for one acceleration structure.
	///
	/// Prebuild sizing and every later build of a structure go through here, so
	/// the description they see can never disagree. Sizing passes a zero
	/// instance address, which it does not read.
	inline D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS accelerationStructureInputs(
		AccelerationStructureType type,
		AccelerationStructureBuildFlags flags,
		std::span<const D3D12_RAYTRACING_GEOMETRY_DESC> geometries,
		std::uint32_t instanceCount,
		D3D12_GPU_VIRTUAL_ADDRESS instanceAddress);
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_RayTracingMappings.inl>
