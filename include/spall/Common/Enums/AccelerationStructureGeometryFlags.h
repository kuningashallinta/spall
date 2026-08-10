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
		Opaque = BIT(0),
		NoDuplicateAnyHitInvocation = BIT(1)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		AccelerationStructureGeometryFlags)
} // namespace spall
