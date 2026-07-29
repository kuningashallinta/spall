#pragma once

#include <cstdint>

namespace spall
{
	struct TextureSubresourceRange
	{
		std::uint32_t BaseMipLevel = 0;
		std::uint32_t MipLevels = 0;
		std::uint32_t BaseArrayLayer = 0;
		std::uint32_t ArrayLayers = 0;
	};
} // namespace spall
