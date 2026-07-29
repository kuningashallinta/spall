namespace spall::vk
{
	inline VkIndexType vulkanIndexType(
		IndexFormat format)
	{
		switch (format)
		{
			case IndexFormat::UInt16:
			{
				return VK_INDEX_TYPE_UINT16;
			}

			case IndexFormat::UInt32:
			default:
			{
				return VK_INDEX_TYPE_UINT32;
			}
		}
	}

	inline std::optional<VertexFormatProperties> vertexFormatInfo(
		Format format)
	{
		if (not isVertexFormat(format))
		{
			return std::nullopt;
		}

		const std::optional<VkFormat> vkFormat = toVkFormat(format);
		const std::uint32_t byteSize = (format == Format::RGB32Float) ? 12 : formatBytesPerPixel(format);

		if ((not vkFormat.has_value()) or (byteSize == 0))
		{
			return std::nullopt;
		}

		return VertexFormatProperties {vkFormat.value(), byteSize};
	}
} // namespace spall::vk
