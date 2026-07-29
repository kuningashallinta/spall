#pragma once

#include <cstdint>

namespace spall::vk
{
	struct TextureFormatProperties
	{
		std::uint32_t bytesPerBlock = 0;
		std::uint32_t blockWidth = 0;
		std::uint32_t blockHeight = 0;
	};
} // namespace spall::vk
