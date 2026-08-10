// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineType.h>
#include <spall/Common/Resource/IResource.h>

namespace spall
{
	class IPipeline : public IResource
	{
	public:
		/// Identifies which bind and dispatch commands accept this pipeline.
		virtual PipelineType type(void) const = 0;
	};
} // namespace spall
