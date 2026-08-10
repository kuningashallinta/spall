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
		AllowUpdate = BIT(0),
		PreferFastTrace = BIT(1),
		PreferFastBuild = BIT(2),
		MinimizeMemory = BIT(3),
		AllowCompaction = BIT(4)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		AccelerationStructureBuildFlags)
} // namespace spall
