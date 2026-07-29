namespace spall
{
	inline std::uint32_t framebufferSampleCount(
		const FramebufferCreateInfo& info)
	{
		if (info.ColorAttachmentCount != 0)
		{
			return info.ColorAttachments[0]->texture().info().SampleCount;
		}

		return (info.DepthAttachment != nullptr) ? info.DepthAttachment->texture().info().SampleCount : 1u;
	}

	inline Status validateFramebufferSampleCounts(
		const FramebufferCreateInfo& info,
		std::uint32_t width,
		std::uint32_t height)
	{
		const std::uint32_t sampleCount = framebufferSampleCount(info);

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < info.ColorAttachmentCount; ++attachmentIndex)
		{
			if (info.ColorAttachments[attachmentIndex]->texture().info().SampleCount != sampleCount)
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		if ((info.DepthAttachment != nullptr) and
			(info.DepthAttachment->texture().info().SampleCount != sampleCount))
		{
			return ERR_INVALID_RESOURCE;
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < info.ColorAttachmentCount; ++attachmentIndex)
		{
			ITextureView* resolveAttachment = info.ResolveAttachments[attachmentIndex];

			if (sampleCount == 1)
			{
				if (resolveAttachment != nullptr)
				{
					return ERR_INVALID_RESOURCE;
				}

				continue;
			}

			if (resolveAttachment == nullptr)
			{
				return ERR_INVALID_RESOURCE;
			}

			const TextureInfo resolveInfo = resolveAttachment->texture().info();

			if (resolveInfo.SampleCount != 1)
			{
				return ERR_INVALID_RESOURCE;
			}

			if (resolveInfo.Format != info.ColorAttachments[attachmentIndex]->texture().info().Format)
			{
				return ERR_INVALID_FORMAT;
			}

			if ((resolveAttachment->mipLevels() != 1) or (resolveAttachment->arrayLayers() != 1) or resolveAttachment->isCubemap())
			{
				return ERR_INVALID_RESOURCE;
			}

			if ((resolveInfo.Usage & TextureUsageFlags::ColorAttachment) == TextureUsageFlags::None)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			if ((mipLevelExtent(resolveInfo.Width, resolveAttachment->baseMipLevel()) != width) or
				(mipLevelExtent(resolveInfo.Height, resolveAttachment->baseMipLevel()) != height))
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		for (std::uint32_t attachmentIndex = info.ColorAttachmentCount; attachmentIndex < MaxColorAttachments; ++attachmentIndex)
		{
			if (info.ResolveAttachments[attachmentIndex] != nullptr)
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		return {};
	}

	inline Status validateFramebufferCreateInfo(
		const FramebufferCreateInfo& info)
	{
		if (info.ColorAttachmentCount > MaxColorAttachments)
		{
			return ERR_INVALID_RESOURCE;
		}

		if ((info.ColorAttachmentCount == 0) and (info.DepthAttachment == nullptr))
		{
			return ERR_INVALID_RESOURCE;
		}

		std::uint32_t framebufferWidth = 0;
		std::uint32_t framebufferHeight = 0;
		bool haveDimensions = false;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < info.ColorAttachmentCount; ++attachmentIndex)
		{
			ITextureView* colorAttachment = info.ColorAttachments[attachmentIndex];

			if (colorAttachment == nullptr)
			{
				return ERR_INVALID_RESOURCE;
			}

			if (colorAttachment->aspects() != TextureAspectFlags::Color)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			const TextureInfo colorTextureInfo = colorAttachment->texture().info();

			if (colorAttachment->mipLevels() != 1)
			{
				return ERR_INVALID_RESOURCE;
			}

			if ((colorAttachment->arrayLayers() != 1) or colorAttachment->isCubemap())
			{
				return ERR_INVALID_RESOURCE;
			}

			if (((colorTextureInfo.Usage & TextureUsageFlags::ColorAttachment) == TextureUsageFlags::None) or
				(not isColorFormat(colorTextureInfo.Format)))
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			const std::uint32_t colorWidth = mipLevelExtent(colorTextureInfo.Width, colorAttachment->baseMipLevel());
			const std::uint32_t colorHeight = mipLevelExtent(colorTextureInfo.Height, colorAttachment->baseMipLevel());

			if (not haveDimensions)
			{
				framebufferWidth = colorWidth;
				framebufferHeight = colorHeight;
				haveDimensions = true;
			}
			else if ((framebufferWidth != colorWidth) or (framebufferHeight != colorHeight))
			{
				return ERR_INVALID_RESOURCE;
			}

			for (std::uint32_t compareIndex = 0; compareIndex < attachmentIndex; ++compareIndex)
			{
				if (&colorAttachment->texture() == &info.ColorAttachments[compareIndex]->texture())
				{
					return ERR_INVALID_RESOURCE;
				}
			}
		}

		if (info.DepthAttachment != nullptr)
		{
			const TextureAspectFlags depthAspects = info.DepthAttachment->aspects();

			if ((depthAspects != TextureAspectFlags::Depth) and
				(depthAspects != (TextureAspectFlags::Depth | TextureAspectFlags::Stencil)))
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			const TextureInfo depthTextureInfo = info.DepthAttachment->texture().info();

			if (info.DepthAttachment->mipLevels() != 1)
			{
				return ERR_INVALID_RESOURCE;
			}

			if ((info.DepthAttachment->arrayLayers() != 1) or info.DepthAttachment->isCubemap())
			{
				return ERR_INVALID_RESOURCE;
			}

			if (((depthTextureInfo.Usage & TextureUsageFlags::DepthStencilAttachment) == TextureUsageFlags::None) or
				(not isDepthFormat(depthTextureInfo.Format)))
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			const std::uint32_t depthWidth = mipLevelExtent(depthTextureInfo.Width, info.DepthAttachment->baseMipLevel());
			const std::uint32_t depthHeight = mipLevelExtent(depthTextureInfo.Height, info.DepthAttachment->baseMipLevel());

			if (not haveDimensions)
			{
				framebufferWidth = depthWidth;
				framebufferHeight = depthHeight;
				haveDimensions = true;
			}
			else if ((framebufferWidth != depthWidth) or (framebufferHeight != depthHeight))
			{
				return ERR_INVALID_RESOURCE;
			}

			for (std::uint32_t attachmentIndex = 0; attachmentIndex < info.ColorAttachmentCount; ++attachmentIndex)
			{
				if (&info.DepthAttachment->texture() == &info.ColorAttachments[attachmentIndex]->texture())
				{
					return ERR_INVALID_RESOURCE;
				}
			}
		}

		return validateFramebufferSampleCounts(info, framebufferWidth, framebufferHeight);
	}
} // namespace spall
