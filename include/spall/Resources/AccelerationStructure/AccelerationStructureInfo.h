// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/AccelerationStructureBuildFlags.h>
#include <spall/Common/Enums/AccelerationStructureType.h>

#include <cstdint>

namespace spall
{
	struct AccelerationStructureInfo
	{
		AccelerationStructureType Type = AccelerationStructureType::BottomLevel;
		AccelerationStructureBuildFlags Flags = AccelerationStructureBuildFlags::None;

		/// Bytes reserved for the built structure.
		std::uint64_t Size = 0;

		/// Scratch bytes a full build consumes. Allocated internally.
		std::uint64_t BuildScratchSize = 0;

		/// Scratch bytes an update consumes, or zero without AllowUpdate.
		std::uint64_t UpdateScratchSize = 0;

		std::uint32_t GeometryCount = 0;
		std::uint32_t InstanceCount = 0;

		const char* DebugName = nullptr;
	};
} // namespace spall
