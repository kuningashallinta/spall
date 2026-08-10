// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace spall
{
	struct Scissor
	{
		std::int32_t X = 0;
		std::int32_t Y = 0;
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
	};
} // namespace spall
