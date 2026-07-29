namespace spall
{
	inline bool isBlockCompressedFormat(
		Format format)
	{
		switch (format)
		{
			case Format::BC1RGBAUnorm:
			case Format::BC1RGBASrgb:
			case Format::BC2RGBAUnorm:
			case Format::BC2RGBASrgb:
			case Format::BC3RGBAUnorm:
			case Format::BC3RGBASrgb:
			case Format::BC4RUnorm:
			case Format::BC4RSnorm:
			case Format::BC5RGUnorm:
			case Format::BC5RGSnorm:
			case Format::BC6HRGBUFloat:
			case Format::BC6HRGBSFloat:
			case Format::BC7RGBAUnorm:
			case Format::BC7RGBASrgb:
			{
				return true;
			}

			default:
			{
				return false;
			}
		}
	}

	inline bool isColorFormat(
		Format format)
	{
		if (isBlockCompressedFormat(format))
		{
			return true;
		}

		switch (format)
		{
			case Format::R8:
			case Format::R8Snorm:
			case Format::R8UInt:
			case Format::R8SInt:
			case Format::R16Unorm:
			case Format::R16Snorm:
			case Format::R16UInt:
			case Format::R16SInt:
			case Format::R16Float:
			case Format::R32Float:
			case Format::R32UInt:
			case Format::R32SInt:
			case Format::RG8Unorm:
			case Format::RG8Snorm:
			case Format::RG8UInt:
			case Format::RG8SInt:
			case Format::RG16Unorm:
			case Format::RG16Snorm:
			case Format::RG16UInt:
			case Format::RG16SInt:
			case Format::RG16Float:
			case Format::RG32Float:
			case Format::RG32UInt:
			case Format::RG32SInt:
			case Format::RGBA16Float:
			case Format::RGBA32Float:
			case Format::RGBA8:
			case Format::RGBA8Snorm:
			case Format::RGBA8UInt:
			case Format::RGBA8SInt:
			case Format::RGBA8Srgb:
			case Format::BGRA8:
			case Format::BGRA8Srgb:
			case Format::RGBA16Unorm:
			case Format::RGBA16Snorm:
			case Format::RGBA16UInt:
			case Format::RGBA16SInt:
			case Format::RGBA32UInt:
			case Format::RGBA32SInt:
			case Format::RGB10A2Unorm:
			case Format::RGB10A2UInt:
			case Format::RG11B10Float:
			case Format::RGB9E5Float:
			{
				return true;
			}

			default:
			{
				return false;
			}
		}
	}

	inline std::uint32_t formatBytesPerPixel(
		Format format)
	{
		switch (format)
		{
			case Format::R8:
			case Format::R8Snorm:
			case Format::R8UInt:
			case Format::R8SInt:
			{
				return 1;
			}

			case Format::R16Unorm:
			case Format::R16Snorm:
			case Format::R16UInt:
			case Format::R16SInt:
			case Format::R16Float:
			case Format::RG8Unorm:
			case Format::RG8Snorm:
			case Format::RG8UInt:
			case Format::RG8SInt:
			case Format::Depth16Unorm:
			{
				return 2;
			}

			case Format::R32Float:
			case Format::R32UInt:
			case Format::R32SInt:
			case Format::RG16Unorm:
			case Format::RG16Snorm:
			case Format::RG16UInt:
			case Format::RG16SInt:
			case Format::RG16Float:
			case Format::RGBA8:
			case Format::RGBA8Snorm:
			case Format::RGBA8UInt:
			case Format::RGBA8SInt:
			case Format::RGBA8Srgb:
			case Format::BGRA8:
			case Format::BGRA8Srgb:
			case Format::RGB10A2Unorm:
			case Format::RGB10A2UInt:
			case Format::RG11B10Float:
			case Format::RGB9E5Float:
			case Format::Depth24Stencil8:
			case Format::Depth32Float:
			{
				return 4;
			}

			case Format::RG32Float:
			case Format::RG32UInt:
			case Format::RG32SInt:
			case Format::RGBA16Unorm:
			case Format::RGBA16Snorm:
			case Format::RGBA16UInt:
			case Format::RGBA16SInt:
			case Format::RGBA16Float:
			case Format::Depth32FloatStencil8:
			{
				return 8;
			}

			case Format::RGBA32Float:
			case Format::RGBA32UInt:
			case Format::RGBA32SInt:
			{
				return 16;
			}

			default:
			{
				return 0;
			}
		}
	}

	inline std::uint32_t formatBlockWidth(
		Format format)
	{
		if (isBlockCompressedFormat(format))
		{
			return 4;
		}

		return isTextureFormat(format) ? 1u : 0u;
	}

	inline std::uint32_t formatBytesPerBlock(
		Format format)
	{
		switch (format)
		{
			case Format::BC1RGBAUnorm:
			case Format::BC1RGBASrgb:
			case Format::BC4RUnorm:
			case Format::BC4RSnorm:
			{
				return 8;
			}

			case Format::BC2RGBAUnorm:
			case Format::BC2RGBASrgb:
			case Format::BC3RGBAUnorm:
			case Format::BC3RGBASrgb:
			case Format::BC5RGUnorm:
			case Format::BC5RGSnorm:
			case Format::BC6HRGBUFloat:
			case Format::BC6HRGBSFloat:
			case Format::BC7RGBAUnorm:
			case Format::BC7RGBASrgb:
			{
				return 16;
			}

			default:
			{
				return formatBytesPerPixel(format);
			}
		}
	}

	inline std::uint32_t formatBlockCount(
		std::uint32_t extent,
		std::uint32_t blockExtent)
	{
		if (blockExtent == 0)
		{
			return 0;
		}

		return (extent + blockExtent - 1u) / blockExtent;
	}
} // namespace spall
