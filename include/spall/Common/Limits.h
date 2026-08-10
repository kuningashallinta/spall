// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace spall
{
	inline constexpr std::uint32_t MaxResourceSets = 4;
	inline constexpr std::uint32_t MaxColorAttachments = 4;
	inline constexpr std::uint32_t MaxPushConstantSize = 128;
	inline constexpr std::uint32_t MaxPatchControlPoints = 32;
	inline constexpr std::uint32_t MaxTextureSampleCount = 64;

	inline constexpr std::uint32_t AccelerationStructureAabbAlignment = 8;
	inline constexpr std::uint32_t MaxAccelerationStructureInstances = 16777216;
	inline constexpr std::uint32_t MaxAccelerationStructureGeometries = 16777216;

	/// Largest value an instance id or user contribution holds.
	inline constexpr std::uint32_t MaxAccelerationStructureInstanceId = 0xFFFFFF;

	/// Largest hit-attribute size a ray-tracing pipeline may declare.
	inline constexpr std::uint32_t MaxRayTracingAttributeSize = 32;

	/// Deepest TraceRay nesting a ray-tracing pipeline may declare.
	inline constexpr std::uint32_t MaxRayRecursionDepth = 31;
} // namespace spall
