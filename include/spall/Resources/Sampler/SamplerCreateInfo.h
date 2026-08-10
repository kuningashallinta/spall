// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/CompareOp.h>
#include <spall/Common/Enums/ResourceEnums.h>

#include <limits>

namespace spall
{
	/// Describes filtering and addressing for sampled textures.
	struct SamplerCreateInfo
	{
		Filter MinFilter = Filter::Linear;
		Filter MagFilter = Filter::Linear;
		Filter MipFilter = Filter::Linear;

		/// Enables hardware depth-reference comparison while sampling.
		bool ComparisonEnabled = false;
		CompareOp Comparison = CompareOp::LessOrEqual;

		AddressMode AddressModeU = AddressMode::Repeat;
		AddressMode AddressModeV = AddressMode::Repeat;
		AddressMode AddressModeW = AddressMode::Repeat;

		/// One disables anisotropic filtering. Larger values clamp to the device limit.
		float MaxAnisotropy = 1.0f;
		float MinLod = 0.0f;

		/// The default leaves the upper mip level unclamped.
		float MaxLod = (std::numeric_limits<float>::max)();
	};
} // namespace spall
