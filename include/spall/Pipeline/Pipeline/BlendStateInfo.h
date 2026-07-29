#pragma once

#include <spall/Common/Enums/PipelineEnums.h>

namespace spall
{
	struct BlendStateInfo
	{
		bool EnableBlend = false;

		BlendFactor SourceColorFactor = BlendFactor::One;
		BlendFactor DestinationColorFactor = BlendFactor::Zero;
		BlendOp ColorBlendOp = BlendOp::Add;

		BlendFactor SourceAlphaFactor = BlendFactor::One;
		BlendFactor DestinationAlphaFactor = BlendFactor::Zero;
		BlendOp AlphaBlendOp = BlendOp::Add;

		ColorComponentFlags ColorWriteMask = ColorComponentFlags::All;
	};
} // namespace spall
