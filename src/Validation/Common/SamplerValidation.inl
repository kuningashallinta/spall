// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Status validateSamplerCreateInfo(
		const SamplerCreateInfo& info)
	{
		if ((info.MinFilter != Filter::Nearest) and (info.MinFilter != Filter::Linear))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.MagFilter != Filter::Nearest) and (info.MagFilter != Filter::Linear))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.MipFilter != Filter::Nearest) and (info.MipFilter != Filter::Linear))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (info.ComparisonEnabled and ((info.Comparison < CompareOp::Never) or (info.Comparison > CompareOp::Always)))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.AddressModeU != AddressMode::ClampToEdge) and (info.AddressModeU != AddressMode::Repeat))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.AddressModeV != AddressMode::ClampToEdge) and (info.AddressModeV != AddressMode::Repeat))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.AddressModeW != AddressMode::ClampToEdge) and (info.AddressModeW != AddressMode::Repeat))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((not std::isfinite(info.MaxAnisotropy)) or (info.MaxAnisotropy < 1.0f))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((not std::isfinite(info.MinLod)) or (info.MinLod < 0.0f))
		{
			return ERR_INVALID_RANGE;
		}

		if (std::isnan(info.MaxLod) or (info.MaxLod < info.MinLod))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}
} // namespace spall
