// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>

#include <spall/Common/Assert.h>

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
			SPALL_ASSERT(alignment > 0);

			const T typedAlignment = static_cast<T>(alignment);
			const T remainder = value % typedAlignment;

			return (remainder == 0) ? value : value + typedAlignment - remainder;
		}
	};
} // namespace spall
