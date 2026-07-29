#pragma once

#include <cstdint>

namespace spall
{
	template <typename Flags>
	constexpr bool hasOnlyKnownFlags(
		Flags value,
		Flags known)
	{
		return (static_cast<std::uint32_t>(value) & ~static_cast<std::uint32_t>(known)) == 0;
	}

	template <typename Flags>
	constexpr bool hasAnyFlag(
		Flags value,
		Flags flag)
	{
		return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
	}
} // namespace spall
