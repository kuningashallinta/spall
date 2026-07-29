namespace spall
{
	inline Status validateSwapChainCreateInfo(
		const SwapChainCreateInfo& info)
	{
		if ((info.Window.Type != WindowHandleType::Win32) or (info.Window.Value == nullptr))
		{
			return ERR_INVALID_WINDOW;
		}

		if ((info.Width == 0) or (info.Height == 0))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.Format == Format::Unknown)
		{
			return ERR_INVALID_FORMAT;
		}

		if (not isRenderTargetFormat(info.Format))
		{
			return ERR_INVALID_FORMAT;
		}

		if ((info.PresentMode != PresentMode::Immediate) and (info.PresentMode != PresentMode::VSync))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.AlphaMode != AlphaMode::Opaque) and (info.AlphaMode != AlphaMode::Premultiplied))
		{
			return ERR_INVALID_ARGUMENT;
		}

		return {};
	}
} // namespace spall
