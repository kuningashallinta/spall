// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace spall
{
	struct QueryPoolInfo
	{
		std::uint32_t TimestampCount = 0;
		const char* DebugName = nullptr;
	};
} // namespace spall
