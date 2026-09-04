// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies how an acceleration structure is built and what it permits afterwards.
	///
	/// AllowUpdate permits refitting the built structure in place. AllowCompaction
	/// makes every build measure its compacted size, which a recorded compaction
	/// then consumes. PreferFastTrace and PreferFastBuild are mutually exclusive.
	enum class AccelerationStructureBuildFlags : std::uint32_t
	{
		None = 0,
		AllowUpdate = SPALL_BIT(0),
		PreferFastTrace = SPALL_BIT(1),
		PreferFastBuild = SPALL_BIT(2),
		MinimizeMemory = SPALL_BIT(3),
		AllowCompaction = SPALL_BIT(4)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		AccelerationStructureBuildFlags)
} // namespace spall
