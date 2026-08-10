// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

namespace spall
{
	struct VertexAttributeInfo
	{
		std::uint32_t Location = 0;
		std::uint32_t Binding = 0;
		spall::Format Format = spall::Format::Unknown;
		std::uint32_t Offset = 0;
	};
} // namespace spall
