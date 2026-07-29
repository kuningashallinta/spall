#pragma once

#include <spall/Common/Enums/AccelerationStructureBuildFlags.h>
#include <spall/Common/Enums/AccelerationStructureType.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureGeometry.h>

#include <cstdint>
#include <span>

namespace spall
{
	class IBuffer;

	/// Describes an acceleration structure to be created by a resource factory.
	///
	/// The shape is fixed at creation: the same description sizes the structure
	/// and drives every later build, so the two can never disagree.
	struct AccelerationStructureCreateInfo
	{
		AccelerationStructureType Type = AccelerationStructureType::BottomLevel;
		AccelerationStructureBuildFlags Flags = AccelerationStructureBuildFlags::PreferFastTrace;

		/// Bottom-level geometry. Must be empty for a top-level structure.
		std::span<const AccelerationStructureGeometry> Geometries;

		/// Top-level array of AccelerationStructureInstance elements.
		/// Must be null for a bottom-level structure.
		IBuffer* InstanceBuffer = nullptr;

		/// Byte offset of the instance array, which must be a multiple of sixteen.
		std::uint32_t InstanceBufferOffset = 0;

		/// Maximum number of instances this structure is sized for.
		std::uint32_t InstanceCount = 0;

		const char* DebugName = nullptr;
	};
} // namespace spall
