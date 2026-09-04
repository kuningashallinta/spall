// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies how one geometry within a bottom-level acceleration structure is traced.
	///
	/// Opaque traces the geometry without any-hit evaluation.
	/// NoDuplicateAnyHitInvocation invokes any-hit at most once per primitive per ray.
	enum class AccelerationStructureGeometryFlags : std::uint32_t
	{
		None = 0,
		Opaque = SPALL_BIT(0),
		NoDuplicateAnyHitInvocation = SPALL_BIT(1)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		AccelerationStructureGeometryFlags)
} // namespace spall
