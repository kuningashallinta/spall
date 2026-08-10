// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/AccelerationStructureInstanceFlags.h>
#include <spall/Common/Limits.h>
#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>

#include <cstdint>

namespace spall
{
	/// GPU layout of one top-level instance, matching the native instance descriptor.
	///
	/// The packed words are written explicitly rather than through bitfields,
	/// whose allocation order is implementation defined. Use
	/// makeAccelerationStructureInstance rather than packing them by hand.
	struct AccelerationStructureInstance
	{
		/// Row-major 3x4 object-to-world transform.
		float Transform[12] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f};

		/// Low 24 bits instance id, high 8 bits visibility mask.
		std::uint32_t InstanceIdAndMask = 0;

		/// Low 24 bits user contribution, high 8 bits AccelerationStructureInstanceFlags.
		std::uint32_t InstanceContributionAndFlags = 0;

		/// IAccelerationStructure::deviceAddress of the referenced bottom-level
		/// structure, which the caller must keep alive.
		std::uint64_t AccelerationStructure = 0;
	};

	/// Byte alignment required of a top-level instance array.
	constexpr std::uint32_t AccelerationStructureInstanceAlignment = 16;

	/// Builds one instance, packing the bitfields the GPU layout requires.
	///
	/// The instance id and contribution are truncated to 24 bits. A zero mask
	/// makes the instance invisible to every ray.
	inline AccelerationStructureInstance makeAccelerationStructureInstance(
		const IAccelerationStructure& bottomLevel,
		const float (&transform)[12],
		std::uint32_t instanceId = 0,
		std::uint8_t instanceMask = 0xFF,
		AccelerationStructureInstanceFlags flags = AccelerationStructureInstanceFlags::None,
		std::uint32_t instanceContribution = 0);
} // namespace spall

#include <spall/Resources/AccelerationStructure/AccelerationStructureInstance.inl>
