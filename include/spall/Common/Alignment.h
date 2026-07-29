#pragma once

#include <cassert>
#include <concepts>

namespace spall
{
	class Alignment
	{
	public:
		template <std::unsigned_integral T, std::integral U>
			requires(not std::same_as<T, bool>) and (not std::same_as<U, bool>)
		static constexpr T up(
			T value,
			U alignment)
		{
			assert(alignment > 0);

			const T typedAlignment = static_cast<T>(alignment);
			const T remainder = value % typedAlignment;
			return (remainder == 0) ? value : value + typedAlignment - remainder;
		}
	};
} // namespace spall
