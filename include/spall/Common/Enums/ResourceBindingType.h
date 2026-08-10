// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	enum class ResourceBindingType
	{
		UniformBuffer,
		SampledTexture,
		StorageBuffer,
		StorageTexture,
		AccelerationStructure
	};
} // namespace spall
