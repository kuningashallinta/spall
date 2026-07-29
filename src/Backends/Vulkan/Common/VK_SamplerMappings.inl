namespace spall::vk
{
	inline VkSamplerAddressMode vulkanSamplerAddressMode(
		AddressMode addressMode)
	{
		switch (addressMode)
		{
			case AddressMode::ClampToEdge:
			{
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			case AddressMode::Repeat:
			default:
			{
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
		}
	}

	inline VkFilter vulkanSamplerFilter(
		Filter filter)
	{
		switch (filter)
		{
			case Filter::Nearest:
			{
				return VK_FILTER_NEAREST;
			}

			case Filter::Linear:
			default:
			{
				return VK_FILTER_LINEAR;
			}
		}
	}
} // namespace spall::vk
