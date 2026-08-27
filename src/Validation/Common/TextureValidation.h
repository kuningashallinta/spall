#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Status/Status.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/Texture/TextureCreateInfo.h>
#include <spall/Resources/Texture/TextureSubresourceRange.h>
#include <spall/Resources/TextureView/TextureViewCreateInfo.h>
#include <src/Validation/Common/FormatValidation.h>

#include <cstdint>

namespace spall
{
	inline std::uint32_t maxTextureMipLevels(
		std::uint32_t width,
		std::uint32_t height);

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

	inline Status validateTextureCreateInfo(const TextureCreateInfo& info);

	inline Status validateTextureViewCreateInfo(const TextureViewCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/TextureValidation.inl>
