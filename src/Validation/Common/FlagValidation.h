#pragma once

namespace spall
{
	template <typename Flags>
	constexpr bool hasOnlyKnownFlags(
		Flags value,
		Flags known)
	{
		return (value & ~known) == Flags {};
	}

	template <typename Flags>
	constexpr bool hasAnyFlag(
		Flags value,
		Flags flag)
	{
		return (value & flag) != Flags {};
	}
} // namespace spall
