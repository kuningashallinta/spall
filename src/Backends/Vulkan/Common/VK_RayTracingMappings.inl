// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline VkAccelerationStructureTypeKHR vulkanAccelerationStructureType(
		AccelerationStructureType type)
	{
		return (type == AccelerationStructureType::TopLevel)
			? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
			: VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	}

	inline VkBuildAccelerationStructureFlagsKHR vulkanAccelerationStructureBuildFlags(
		AccelerationStructureBuildFlags flags)
	{
		VkBuildAccelerationStructureFlagsKHR nativeFlags = 0;

		if ((flags & AccelerationStructureBuildFlags::AllowUpdate) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}

		if ((flags & AccelerationStructureBuildFlags::PreferFastTrace) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		}

		if ((flags & AccelerationStructureBuildFlags::PreferFastBuild) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		}

		if ((flags & AccelerationStructureBuildFlags::MinimizeMemory) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
		}

		if ((flags & AccelerationStructureBuildFlags::AllowCompaction) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		}

		return nativeFlags;
	}

	inline VkGeometryFlagsKHR vulkanAccelerationStructureGeometryFlags(
		AccelerationStructureGeometryFlags flags)
	{
		VkGeometryFlagsKHR nativeFlags = 0;

		if ((flags & AccelerationStructureGeometryFlags::Opaque) != AccelerationStructureGeometryFlags::None)
		{
			nativeFlags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
		}

		if ((flags & AccelerationStructureGeometryFlags::NoDuplicateAnyHitInvocation) != AccelerationStructureGeometryFlags::None)
		{
			nativeFlags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
		}

		return nativeFlags;
	}

	inline bool isSupportedAccelerationStructureVertexFormat(
		Format format)
	{
		return (format == Format::RGB32Float) or (format == Format::RG32Float) or
			(format == Format::RGBA16Float) or (format == Format::RG16Float) or
			(format == Format::RGBA16Snorm) or (format == Format::RG16Snorm);
	}
} // namespace spall::vk
