// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Status validatePushConstantUpdate(
		ShaderStageFlags declaredStages,
		std::uint32_t declaredSize,
		ShaderStageFlags stages,
		std::uint32_t offset,
		std::size_t size)
	{
		if ((declaredSize == 0) or (declaredStages == ShaderStageFlags::None))
		{
			return ERR_INVALID_STATE;
		}

		if (stages != declaredStages)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		if ((size == 0) or ((offset % 4) != 0) or ((size % 4) != 0))
		{
			return ERR_INVALID_SIZE;
		}

		const std::uint64_t end = static_cast<std::uint64_t>(offset) + static_cast<std::uint64_t>(size);

		if (end > declaredSize)
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateViewport(
		const Viewport& viewport)
	{
		if ((not std::isfinite(viewport.X)) or (not std::isfinite(viewport.Y)) or
			(not std::isfinite(viewport.Width)) or (not std::isfinite(viewport.Height)) or
			(not std::isfinite(viewport.MinDepth)) or (not std::isfinite(viewport.MaxDepth)))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((viewport.Width <= 0.0f) or (viewport.Height <= 0.0f))
		{
			return ERR_INVALID_SIZE;
		}

		if ((not std::isfinite(viewport.X + viewport.Width)) or
			(not std::isfinite(viewport.Y + viewport.Height)))
		{
			return ERR_INVALID_RANGE;
		}

		if ((viewport.MinDepth < 0.0f) or (viewport.MaxDepth > 1.0f) or
			(viewport.MinDepth > viewport.MaxDepth))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateScissor(
		const Scissor& scissor)
	{
		if ((scissor.X < 0) or (scissor.Y < 0))
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint64_t right = static_cast<std::uint64_t>(scissor.X) + static_cast<std::uint64_t>(scissor.Width);
		const std::uint64_t bottom = static_cast<std::uint64_t>(scissor.Y) + static_cast<std::uint64_t>(scissor.Height);
		const std::uint64_t maximumCoordinate = static_cast<std::uint64_t>((std::numeric_limits<std::int32_t>::max)());

		if ((right > maximumCoordinate) or (bottom > maximumCoordinate))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateCopyBufferArguments(
		const IBuffer& destination,
		std::uint32_t destinationOffset,
		const IBuffer& source,
		std::uint32_t sourceOffset,
		std::uint32_t byteSize)
	{
		if (&destination == &source)
		{
			return ERR_INVALID_RESOURCE;
		}

		if (byteSize == 0)
		{
			return ERR_INVALID_SIZE;
		}

		const std::uint64_t destinationEnd = static_cast<std::uint64_t>(destinationOffset) + static_cast<std::uint64_t>(byteSize);
		const std::uint64_t sourceEnd = static_cast<std::uint64_t>(sourceOffset) + static_cast<std::uint64_t>(byteSize);

		if ((destinationEnd > destination.info().Size) or (sourceEnd > source.info().Size))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateIndirectArguments(
		const IBuffer& argumentBuffer,
		std::uint32_t offset,
		std::uint32_t argumentSize)
	{
		const BufferInfo info = argumentBuffer.info();

		if ((info.Usage & BufferUsageFlags::Indirect) == BufferUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((offset % IndirectArgumentAlignment) != 0)
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint64_t argumentEnd = static_cast<std::uint64_t>(offset) + static_cast<std::uint64_t>(argumentSize);

		if (argumentEnd > info.Size)
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline TextureRegion resolveTextureRegion(
		const TextureInfo& info,
		const TextureRegion& region)
	{
		const std::uint32_t mipWidth = mipLevelExtent(info.Width, region.MipLevel);
		const std::uint32_t mipHeight = mipLevelExtent(info.Height, region.MipLevel);
		const std::uint32_t mipDepth = mipLevelExtent(info.Depth, region.MipLevel);

		TextureRegion resolved = region;
		resolved.Width = (region.Width != 0) ? region.Width : ((region.X < mipWidth) ? (mipWidth - region.X) : 0);
		resolved.Height = (region.Height != 0) ? region.Height : ((region.Y < mipHeight) ? (mipHeight - region.Y) : 0);
		resolved.Depth = (region.Depth != 0) ? region.Depth : ((region.Z < mipDepth) ? (mipDepth - region.Z) : 0);

		return resolved;
	}

	inline Status validateTextureBufferCopyArguments(
		const ITexture& texture,
		const TextureRegion& region,
		const IBuffer& buffer,
		std::uint32_t bufferOffset,
		std::uint32_t bufferRowPitch)
	{
		if ((texture.info().Width == 0) or (texture.info().Height == 0))
		{
			return ERR_INVALID_SIZE;
		}

		if (region.MipLevel >= texture.info().MipLevels)
		{
			return ERR_INVALID_RANGE;
		}

		if (region.ArrayLayer >= texture.info().ArrayLayers)
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint32_t textureWidth = mipLevelExtent(texture.info().Width, region.MipLevel);
		const std::uint32_t textureHeight = mipLevelExtent(texture.info().Height, region.MipLevel);
		const std::uint32_t textureDepth = mipLevelExtent(texture.info().Depth, region.MipLevel);
		const TextureRegion resolved = resolveTextureRegion(texture.info(), region);

		if ((resolved.Width == 0) or (resolved.Height == 0) or (resolved.Depth == 0))
		{
			return ERR_INVALID_SIZE;
		}

		const std::uint64_t textureRight = static_cast<std::uint64_t>(resolved.X) + resolved.Width;
		const std::uint64_t textureBottom = static_cast<std::uint64_t>(resolved.Y) + resolved.Height;
		const std::uint64_t textureBack = static_cast<std::uint64_t>(resolved.Z) + resolved.Depth;

		if ((textureRight > textureWidth) or (textureBottom > textureHeight) or (textureBack > textureDepth))
		{
			return ERR_INVALID_RANGE;
		}

		if ((bufferOffset >= buffer.info().Size) or (bufferRowPitch == 0))
		{
			return ERR_INVALID_RANGE;
		}

		const Format format = texture.info().Format;
		const std::uint32_t blockWidth = formatBlockWidth(format);
		const std::uint32_t blockHeight = formatBlockHeight(format);
		const std::uint32_t bytesPerBlock = formatBytesPerBlock(format);

		if ((blockWidth == 0) or (blockHeight == 0) or (bytesPerBlock == 0))
		{
			return ERR_INVALID_FORMAT;
		}

		if (((resolved.X % blockWidth) != 0) or ((resolved.Y % blockHeight) != 0))
		{
			return ERR_INVALID_RANGE;
		}

		if ((((resolved.Width % blockWidth) != 0) and (textureRight != textureWidth)) or
			(((resolved.Height % blockHeight) != 0) and (textureBottom != textureHeight)))
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint64_t minimumRowPitch = static_cast<std::uint64_t>(formatBlockCount(resolved.Width, blockWidth)) * static_cast<std::uint64_t>(bytesPerBlock);

		if (static_cast<std::uint64_t>(bufferRowPitch) < minimumRowPitch)
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint64_t availableBytes = static_cast<std::uint64_t>(buffer.info().Size) - static_cast<std::uint64_t>(bufferOffset);
		const std::uint64_t precedingRowsBytes = static_cast<std::uint64_t>(bufferRowPitch) *
			static_cast<std::uint64_t>(formatBlockCount(resolved.Height, blockHeight) - 1u);

		if ((precedingRowsBytes > availableBytes) or (minimumRowPitch > (availableBytes - precedingRowsBytes)))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateCopyTextureArguments(
		const ITexture& destination,
		const ITexture& source)
	{
		if (&destination == &source)
		{
			return ERR_INVALID_RESOURCE;
		}

		if (((destination.info().Usage & TextureUsageFlags::TransferDestination) == TextureUsageFlags::None) or
			((source.info().Usage & TextureUsageFlags::TransferSource) == TextureUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((destination.info().Width != source.info().Width) or (destination.info().Height != source.info().Height) or
			(destination.info().MipLevels != source.info().MipLevels) or
			(destination.info().ArrayLayers != source.info().ArrayLayers) or
			(destination.info().Format != source.info().Format))
		{
			return ERR_INVALID_RESOURCE;
		}

		return {};
	}

	inline Status validateGenerateMipsArguments(
		const ITexture& texture)
	{
		const TextureInfo info = texture.info();

		if (not isColorFormat(info.Format))
		{
			return ERR_INVALID_FORMAT;
		}

		if (isBlockCompressedFormat(info.Format))
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		if (info.MipLevels < 2)
		{
			return ERR_INVALID_RANGE;
		}

		constexpr TextureUsageFlags requiredUsage =
			TextureUsageFlags::Sampled |
			TextureUsageFlags::TransferSource |
			TextureUsageFlags::TransferDestination;

		if ((info.Usage & requiredUsage) != requiredUsage)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		return {};
	}
} // namespace spall
