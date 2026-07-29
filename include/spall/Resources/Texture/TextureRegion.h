#pragma once

#include <cstdint>

namespace spall
{
	struct TextureRegion
	{
		std::uint32_t MipLevel = 0;
		std::uint32_t X = 0;
		std::uint32_t Y = 0;

		// A zero extent selects the remainder of the mip level.
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
		std::uint32_t ArrayLayer = 0;

		std::uint32_t Z = 0;
		std::uint32_t Depth = 0;
	};
} // namespace spall
