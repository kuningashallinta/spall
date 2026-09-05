// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Status/Status.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/Texture/Texture1DCreateInfo.h>
#include <spall/Resources/Texture/Texture2DCreateInfo.h>
#include <spall/Resources/Texture/Texture3DCreateInfo.h>
#include <spall/Resources/Texture/TextureSubresourceRange.h>
#include <spall/Resources/TextureView/TextureViewCreateInfo.h>
#include <src/Validation/Common/FormatValidation.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>

namespace spall
{
	inline std::uint32_t maxTextureMipLevels(
		std::uint32_t width,
		std::uint32_t height,
		std::uint32_t depth);

	inline std::uint32_t mipLevelExtent(
		std::uint32_t baseExtent,
		std::uint32_t mipLevel);

	constexpr std::uint32_t CubemapFaceCount = 6;

	inline bool isValidSampleCount(
		std::uint32_t sampleCount)
	{
		return (sampleCount != 0) and (sampleCount <= MaxTextureSampleCount) and
			((sampleCount & (sampleCount - 1)) == 0);
	}

	inline std::uint32_t textureSubresourceCount(
		const TextureInfo& info)
	{
		return info.MipLevels * info.ArrayLayers;
	}

	inline std::uint32_t textureSubresourceIndex(
		const TextureInfo& info,
		std::uint32_t mipLevel,
		std::uint32_t arrayLayer)
	{
		return (mipLevel * info.ArrayLayers) + arrayLayer;
	}

	inline TextureSubresourceRange resolveTextureSubresourceRange(
		const TextureInfo& info,
		const TextureSubresourceRange& range);

	inline Status validateTextureSubresourceRange(
		const TextureInfo& info,
		const TextureSubresourceRange& range);

	/// Validates the format and usage rules shared by every texture dimension.
	inline Status validateTextureFormatUsage(
		Format format,
		TextureUsageFlags usage);

	inline Status validateTextureInitialState(
		ResourceStateFlags initialState,
		TextureUsageFlags usage);

	inline Status validateTexture1DCreateInfo(const Texture1DCreateInfo& info);

	inline Status validateTexture2DCreateInfo(const Texture2DCreateInfo& info);

	inline Status validateTexture3DCreateInfo(const Texture3DCreateInfo& info);

	inline Status validateTextureViewCreateInfo(const TextureViewCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/TextureValidation.inl>
