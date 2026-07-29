#pragma once

#include <spall/Common/Color/Color.h>

#include <cstdint>

namespace spall
{
	/// Describes the clears performed by a managed frame's default render pass.
	struct FrameBeginInfo
	{
		Color ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		float ClearDepth = 1.0f;
		std::uint8_t ClearStencil = 0;
	};
} // namespace spall
