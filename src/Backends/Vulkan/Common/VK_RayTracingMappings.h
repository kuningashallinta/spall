#pragma once

#include <spall/Common/Enums/ResourceEnums.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	inline VkAccelerationStructureTypeKHR vulkanAccelerationStructureType(
		AccelerationStructureType type);

	inline VkBuildAccelerationStructureFlagsKHR vulkanAccelerationStructureBuildFlags(
		AccelerationStructureBuildFlags flags);

	inline VkGeometryFlagsKHR vulkanAccelerationStructureGeometryFlags(
		AccelerationStructureGeometryFlags flags);

	inline bool isSupportedAccelerationStructureVertexFormat(
		Format format);
} // namespace spall::vk

#include <src/Backends/Vulkan/Common/VK_RayTracingMappings.inl>
