// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <limits>
#include <utility>

#include <spall/Common/Assert.h>
#include <spall/Common/Status/Status.h>

namespace spall
{
	class Alignment
	{
	public:
		/// Rounds up to the next multiple of the alignment, saturating at the
		/// largest representable value rather than wrapping past it.
		template <std::unsigned_integral T, std::integral U>
			requires(not std::same_as<T, bool>) and (not std::same_as<U, bool>)
		static constexpr T up(
			T value,
			U alignment)
		{
			SPALL_VERIFY(alignment > 0);
			SPALL_VERIFY(std::cmp_less_equal(alignment, (std::numeric_limits<T>::max)()));

			T result = 0;

			if (up(value, alignment, &result) == ERR_INVALID_SIZE)
			{
				return (std::numeric_limits<T>::max)();
			}

			return result;
		}

		/// Rounds up to the next multiple of the alignment, reporting failure
		/// instead of saturating or aborting.
		///
		/// Reports ERR_INVALID_ARGUMENT for a null result or an alignment that is
		/// not positive or not representable in T, and ERR_INVALID_SIZE when the
		/// rounded value does not fit. Leaves result untouched on failure.
		template <std::unsigned_integral T, std::integral U>
			requires(not std::same_as<T, bool>) and (not std::same_as<U, bool>)
		static constexpr Status up(
			T value,
			U alignment,
			T* result)
		{
			if (result == nullptr)
			{
				return ERR_INVALID_ARGUMENT;
			}

			if (std::cmp_less_equal(alignment, 0) or
				std::cmp_greater(alignment, (std::numeric_limits<T>::max)()))
			{
				return ERR_INVALID_ARGUMENT;
			}

			const T typedAlignment = static_cast<T>(alignment);
			const T remainder = value % typedAlignment;

			if (remainder == 0)
			{
				*result = value;

				return {};
			}

			const T padding = typedAlignment - remainder;

			if (padding > ((std::numeric_limits<T>::max)() - value))
			{
				return ERR_INVALID_SIZE;
			}

			*result = value + padding;

			return {};
		}
	};
} // namespace spall
