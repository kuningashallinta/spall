namespace spall
{
	inline std::uint32_t maxTextureMipLevels(
		std::uint32_t width,
		std::uint32_t height)
	{
		std::uint32_t largestDimension = (width > height) ? width : height;
		std::uint32_t levels = 1;

		while (largestDimension > 1)
		{
			largestDimension /= 2;
			++levels;
		}

		return levels;
	}

	inline std::uint32_t mipLevelExtent(
		std::uint32_t baseExtent,
		std::uint32_t mipLevel)
	{
		const std::uint32_t extent = (mipLevel < 32u) ? (baseExtent >> mipLevel) : 0u;

		return (extent != 0) ? extent : 1u;
	}

	inline TextureSubresourceRange resolveTextureSubresourceRange(
		const TextureInfo& info,
		const TextureSubresourceRange& range)
	{
		TextureSubresourceRange resolved = range;

		resolved.MipLevels = (range.MipLevels != 0)
			? range.MipLevels
			: ((range.BaseMipLevel < info.MipLevels) ? (info.MipLevels - range.BaseMipLevel) : 0);
		resolved.ArrayLayers = (range.ArrayLayers != 0)
			? range.ArrayLayers
			: ((range.BaseArrayLayer < info.ArrayLayers) ? (info.ArrayLayers - range.BaseArrayLayer) : 0);

		return resolved;
	}

	inline Status validateTextureSubresourceRange(
		const TextureInfo& info,
		const TextureSubresourceRange& range)
	{
		const TextureSubresourceRange resolved = resolveTextureSubresourceRange(info, range);

		if ((resolved.MipLevels == 0) or (resolved.ArrayLayers == 0))
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint64_t lastMipLevel = static_cast<std::uint64_t>(resolved.BaseMipLevel) + static_cast<std::uint64_t>(resolved.MipLevels);
		const std::uint64_t lastArrayLayer = static_cast<std::uint64_t>(resolved.BaseArrayLayer) + static_cast<std::uint64_t>(resolved.ArrayLayers);

		if ((lastMipLevel > info.MipLevels) or (lastArrayLayer > info.ArrayLayers))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateTextureCreateInfo(
		const TextureCreateInfo& info)
	{
		constexpr std::uint32_t knownUsageMask =
			static_cast<std::uint32_t>(TextureUsageFlags::ColorAttachment) |
			static_cast<std::uint32_t>(TextureUsageFlags::DepthStencilAttachment) |
			static_cast<std::uint32_t>(TextureUsageFlags::TransferSource) |
			static_cast<std::uint32_t>(TextureUsageFlags::TransferDestination) |
			static_cast<std::uint32_t>(TextureUsageFlags::Sampled) |
			static_cast<std::uint32_t>(TextureUsageFlags::Storage);

		if ((info.Width == 0) or (info.Height == 0) or (info.Depth == 0))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.MipLevels == 0)
		{
			return ERR_INVALID_SIZE;
		}

		if (info.MipLevels > maxTextureMipLevels(info.Width, info.Height))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.ArrayLayers == 0)
		{
			return ERR_INVALID_SIZE;
		}

		if (not isValidSampleCount(info.SampleCount))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.SampleCount > 1)
		{
			constexpr TextureUsageFlags attachmentUsage = TextureUsageFlags::ColorAttachment | TextureUsageFlags::DepthStencilAttachment;

			if ((info.Usage & attachmentUsage) == TextureUsageFlags::None)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if ((info.Usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if (info.MipLevels != 1)
			{
				return ERR_INVALID_SIZE;
			}

			if (info.Cubemap)
			{
				return ERR_UNSUPPORTED_USAGE;
			}
		}

		if (info.Cubemap)
		{
			if ((info.ArrayLayers % CubemapFaceCount) != 0)
			{
				return ERR_INVALID_SIZE;
			}

			if (info.Width != info.Height)
			{
				return ERR_INVALID_SIZE;
			}
		}

		if (info.Depth > 1)
		{
			if (info.ArrayLayers != 1)
			{
				return ERR_INVALID_SIZE;
			}

			if (info.Cubemap)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if (info.SampleCount != 1)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if ((info.Usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}
		}

		if (info.Format == Format::Unknown)
		{
			return ERR_INVALID_FORMAT;
		}

		if (not isTextureFormat(info.Format))
		{
			return ERR_INVALID_FORMAT;
		}

		if (info.Usage == TextureUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((static_cast<std::uint32_t>(info.Usage) & ~knownUsageMask) != 0)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		const bool hasColorUsage = ((info.Usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None);
		const bool hasDepthUsage = ((info.Usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None);

		if (hasColorUsage and hasDepthUsage)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (isDepthFormat(info.Format) and hasColorUsage)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((not isDepthFormat(info.Format)) and hasDepthUsage)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (isDepthFormat(info.Format) and ((info.Usage & TextureUsageFlags::Sampled) != TextureUsageFlags::None))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if (isDepthFormat(info.Format) and ((info.Usage & TextureUsageFlags::Storage) != TextureUsageFlags::None))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if (isBlockCompressedFormat(info.Format))
		{
			if (hasColorUsage)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if ((info.Usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if (((info.Width % formatBlockWidth(info.Format)) != 0) or ((info.Height % formatBlockHeight(info.Format)) != 0))
			{
				return ERR_INVALID_SIZE;
			}
		}

		const std::uint32_t initialState = static_cast<std::uint32_t>(info.InitialState);
		constexpr std::uint32_t supportedTextureStateMask =
			static_cast<std::uint32_t>(ResourceStateFlags::Common) |
			static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource) |
			static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess) |
			static_cast<std::uint32_t>(ResourceStateFlags::RenderTarget) |
			static_cast<std::uint32_t>(ResourceStateFlags::DepthWrite) |
			static_cast<std::uint32_t>(ResourceStateFlags::DepthRead) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopySource) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopyDest);

		if ((initialState == 0) or ((initialState & ~supportedTextureStateMask) != 0))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		switch (info.InitialState)
		{
			case ResourceStateFlags::Common:
			{
				break;
			}

			case ResourceStateFlags::ShaderResource:
			{
				if ((info.Usage & TextureUsageFlags::Sampled) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			case ResourceStateFlags::UnorderedAccess:
			{
				if ((info.Usage & TextureUsageFlags::Storage) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			case ResourceStateFlags::RenderTarget:
			{
				if ((info.Usage & TextureUsageFlags::ColorAttachment) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			case ResourceStateFlags::DepthWrite:
			{
				if ((info.Usage & TextureUsageFlags::DepthStencilAttachment) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			case ResourceStateFlags::DepthRead:
			{
				if ((info.Usage & TextureUsageFlags::DepthStencilAttachment) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			case ResourceStateFlags::CopySource:
			{
				if ((info.Usage & TextureUsageFlags::TransferSource) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			case ResourceStateFlags::CopyDest:
			{
				if ((info.Usage & TextureUsageFlags::TransferDestination) == TextureUsageFlags::None)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}

				break;
			}

			default:
			{
				return ERR_INVALID_RESOURCE_STATE;
			}
		}

		return {};
	}

	inline Status validateTextureViewCreateInfo(
		const TextureViewCreateInfo& info)
	{
		constexpr std::uint32_t knownAspectMask =
			static_cast<std::uint32_t>(TextureAspectFlags::Color) |
			static_cast<std::uint32_t>(TextureAspectFlags::Depth) |
			static_cast<std::uint32_t>(TextureAspectFlags::Stencil);

		if (info.Texture == nullptr)
		{
			return ERR_INVALID_RESOURCE;
		}

		if ((static_cast<std::uint32_t>(info.Aspects) & ~knownAspectMask) != 0)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		const TextureInfo textureInfo = info.Texture->info();
		const Format format = (info.Format == Format::Unknown) ? textureInfo.Format : info.Format;

		if (info.BaseMipLevel >= textureInfo.MipLevels)
		{
			return ERR_INVALID_RANGE;
		}

		if ((info.MipLevels != 0) and (info.MipLevels > textureInfo.MipLevels - info.BaseMipLevel))
		{
			return ERR_INVALID_RANGE;
		}

		if (info.BaseArrayLayer >= textureInfo.ArrayLayers)
		{
			return ERR_INVALID_RANGE;
		}

		if ((info.ArrayLayers != 0) and (info.ArrayLayers > textureInfo.ArrayLayers - info.BaseArrayLayer))
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint32_t viewArrayLayers = (info.ArrayLayers != 0)
			? info.ArrayLayers
			: (textureInfo.ArrayLayers - info.BaseArrayLayer);

		if (info.Cubemap)
		{
			if (not textureInfo.Cubemap)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if (((info.BaseArrayLayer % CubemapFaceCount) != 0) or ((viewArrayLayers % CubemapFaceCount) != 0))
			{
				return ERR_INVALID_RANGE;
			}
		}

		if ((info.Format != Format::Unknown) and (info.Format != textureInfo.Format))
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		if (format == Format::Unknown)
		{
			return ERR_INVALID_FORMAT;
		}

		if (not isTextureFormat(format))
		{
			return ERR_INVALID_FORMAT;
		}

		const TextureAspectFlags defaultAspects = hasStencilAspect(format)
			? (TextureAspectFlags::Depth | TextureAspectFlags::Stencil)
			: (isDepthFormat(format) ? TextureAspectFlags::Depth : TextureAspectFlags::Color);

		const TextureAspectFlags aspects = (info.Aspects == TextureAspectFlags::None) ? defaultAspects : info.Aspects;

		const bool usesColorAspect = ((aspects & TextureAspectFlags::Color) != TextureAspectFlags::None);
		const bool usesDepthAspect = ((aspects & TextureAspectFlags::Depth) != TextureAspectFlags::None);
		const bool usesStencilAspect = ((aspects & TextureAspectFlags::Stencil) != TextureAspectFlags::None);

		if (isDepthFormat(format) and usesColorAspect)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((not isDepthFormat(format)) and (usesDepthAspect or usesStencilAspect))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((not hasStencilAspect(format)) and usesStencilAspect)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (usesColorAspect and
			((textureInfo.Usage & (TextureUsageFlags::ColorAttachment | TextureUsageFlags::Sampled)) == TextureUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((usesDepthAspect or usesStencilAspect) and ((textureInfo.Usage & TextureUsageFlags::DepthStencilAttachment) == TextureUsageFlags::None))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		return {};
	}
} // namespace spall
