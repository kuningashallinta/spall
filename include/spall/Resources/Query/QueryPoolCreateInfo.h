#pragma once

#include <cstdint>

namespace spall
{
	struct QueryPoolCreateInfo
	{
		std::uint32_t TimestampCount = 0;
		const char* DebugName = nullptr;
	};
} // namespace spall
