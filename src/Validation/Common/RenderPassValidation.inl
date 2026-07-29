namespace spall
{
	inline Status validatePassBeginInfo(
		const RenderPassBeginInfo& beginInfo)
	{
		if (beginInfo.Framebuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE;
		}

		const FramebufferInfo framebufferInfo = beginInfo.Framebuffer->info();

		if ((framebufferInfo.ColorFormatCount > MaxColorAttachments) or
			((framebufferInfo.ColorFormatCount == 0) and (framebufferInfo.DepthFormat == Format::Unknown)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebufferInfo.ColorFormatCount; ++attachmentIndex)
		{
			const ColorAttachmentInfo& attachment = beginInfo.ColorAttachments[attachmentIndex];

			if ((attachment.LoadAction != LoadAction::Load) and
				(attachment.LoadAction != LoadAction::Clear) and
				(attachment.LoadAction != LoadAction::DontCare))
			{
				return ERR_INVALID_ARGUMENT;
			}

			if ((attachment.StoreAction != StoreAction::Store) and
				(attachment.StoreAction != StoreAction::DontCare))
			{
				return ERR_INVALID_ARGUMENT;
			}

			if ((attachment.LoadAction == LoadAction::Clear) and
				((not std::isfinite(attachment.ClearColor.R)) or
					(not std::isfinite(attachment.ClearColor.G)) or
					(not std::isfinite(attachment.ClearColor.B)) or
					(not std::isfinite(attachment.ClearColor.A))))
			{
				return ERR_INVALID_ARGUMENT;
			}
		}

		if (framebufferInfo.DepthFormat != Format::Unknown)
		{
			const DepthStencilAttachmentInfo& attachment = beginInfo.DepthAttachment;

			if (((attachment.DepthLoadAction != LoadAction::Load) and
					(attachment.DepthLoadAction != LoadAction::Clear) and
					(attachment.DepthLoadAction != LoadAction::DontCare)) or
				((attachment.StencilLoadAction != LoadAction::Load) and
					(attachment.StencilLoadAction != LoadAction::Clear) and
					(attachment.StencilLoadAction != LoadAction::DontCare)))
			{
				return ERR_INVALID_ARGUMENT;
			}

			if (((attachment.DepthStoreAction != StoreAction::Store) and
					(attachment.DepthStoreAction != StoreAction::DontCare)) or
				((attachment.StencilStoreAction != StoreAction::Store) and
					(attachment.StencilStoreAction != StoreAction::DontCare)))
			{
				return ERR_INVALID_ARGUMENT;
			}

			if ((attachment.DepthLoadAction == LoadAction::Clear) and
				((not std::isfinite(attachment.ClearDepth)) or
					(attachment.ClearDepth < 0.0f) or
					(attachment.ClearDepth > 1.0f)))
			{
				return ERR_INVALID_RANGE;
			}

			if ((not hasStencilAspect(framebufferInfo.DepthFormat)) and
				(attachment.StencilLoadAction == LoadAction::Clear))
			{
				return ERR_INVALID_FORMAT;
			}
		}

		return {};
	}
} // namespace spall
