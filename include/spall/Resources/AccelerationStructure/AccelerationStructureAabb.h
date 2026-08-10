// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	/// GPU layout of one procedural bounding box, matching the native descriptor.
	struct AccelerationStructureAabb
	{
		float MinX = 0.0f;
		float MinY = 0.0f;
		float MinZ = 0.0f;
		float MaxX = 0.0f;
		float MaxY = 0.0f;
		float MaxZ = 0.0f;
	};
} // namespace spall
