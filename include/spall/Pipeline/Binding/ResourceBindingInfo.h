// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

namespace spall
{
	struct ResourceBindingInfo
	{
		std::uint32_t Binding = 0;
		ResourceBindingType Type = ResourceBindingType::UniformBuffer;
		ShaderStageFlags Stages = ShaderStageFlags::None;
	};
} // namespace spall
