// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	/// Identifies which kind of work a pipeline records.
	enum class PipelineType
	{
		Graphics,
		Compute,
		RayTracing
	};
} // namespace spall
