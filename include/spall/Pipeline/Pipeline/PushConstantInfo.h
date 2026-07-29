#pragma once

#include <spall/Common/Enums/ShaderStageFlags.h>
#include <spall/Common/Limits.h>

#include <cstdint>

namespace spall
{
	/// Declares one portable push-constant block. HLSL shaders access it through register b13.
	struct PushConstantInfo
	{
		ShaderStageFlags Stages = ShaderStageFlags::None;

		/// Must be a multiple of four and no larger than MaxPushConstantSize.
		std::uint32_t Size = 0;
	};
} // namespace spall
