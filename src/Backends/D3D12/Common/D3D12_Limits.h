// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <limits>

namespace spall::d3d12
{
	inline constexpr std::uint32_t RenderTargetViewHeapCapacity = 256;
	inline constexpr std::uint32_t DepthStencilViewHeapCapacity = 128;
	inline constexpr std::uint32_t ShaderResourceHeapCapacity = 4096;
	inline constexpr std::uint32_t SamplerHeapCapacity = 512;

	inline constexpr std::uint32_t DescriptorRingViewCapacity = 4096;
	inline constexpr std::uint32_t DescriptorRingSamplerCapacity = 512;

	inline constexpr std::uint32_t InvalidDescriptorIndex = (std::numeric_limits<std::uint32_t>::max)();
	inline constexpr std::uint32_t InvalidRootParameter = (std::numeric_limits<std::uint32_t>::max)();
} // namespace spall::d3d12
