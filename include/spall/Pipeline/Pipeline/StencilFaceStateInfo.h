// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>

namespace spall
{
	struct StencilFaceStateInfo
	{
		StencilOp FailOp = StencilOp::Keep;
		StencilOp DepthFailOp = StencilOp::Keep;
		StencilOp PassOp = StencilOp::Keep;
		CompareOp Compare = CompareOp::Always;
	};
} // namespace spall
