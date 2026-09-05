// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Resources/Query/IQueryPool.h>
#include <spall/Resources/Query/QueryPoolCreateInfo.h>

#include <cstdint>

namespace spall
{
	inline Status validateQueryPoolCreateInfo(const QueryPoolCreateInfo& info);

	inline Status validateTimestampWrite(
		const QueryPoolInfo& info,
		std::uint32_t query);

	inline Status validateTimestampRead(
		const QueryPoolInfo& info,
		std::uint32_t firstQuery,
		std::size_t queryCount);
} // namespace spall

#include <src/Validation/Common/QueryValidation.inl>
