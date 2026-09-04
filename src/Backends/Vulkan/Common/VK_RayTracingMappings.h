// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>

#include <vulkan/vulkan.h>

namespace spall::vk
{
	inline VkAccelerationStructureTypeKHR accelerationStructureType(AccelerationStructureType type);

	inline VkBuildAccelerationStructureFlagsKHR accelerationStructureBuildFlags(AccelerationStructureBuildFlags flags);

	inline VkGeometryFlagsKHR accelerationStructureGeometryFlags(AccelerationStructureGeometryFlags flags);

	inline bool isSupportedAccelerationStructureVertexFormat(Format format);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_RayTracingMappings.inl>
