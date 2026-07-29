namespace spall::dxgi
{
	inline DXGI_FORMAT nativeFormat(
		Format format)
	{
		switch (format)
		{
			case Format::R8:
			{
				return DXGI_FORMAT_R8_UNORM;
			}

			case Format::R8Snorm:
			{
				return DXGI_FORMAT_R8_SNORM;
			}

			case Format::R8UInt:
			{
				return DXGI_FORMAT_R8_UINT;
			}

			case Format::R8SInt:
			{
				return DXGI_FORMAT_R8_SINT;
			}

			case Format::R16Unorm:
			{
				return DXGI_FORMAT_R16_UNORM;
			}

			case Format::R16Snorm:
			{
				return DXGI_FORMAT_R16_SNORM;
			}

			case Format::R16UInt:
			{
				return DXGI_FORMAT_R16_UINT;
			}

			case Format::R16SInt:
			{
				return DXGI_FORMAT_R16_SINT;
			}

			case Format::R16Float:
			{
				return DXGI_FORMAT_R16_FLOAT;
			}

			case Format::R32Float:
			{
				return DXGI_FORMAT_R32_FLOAT;
			}

			case Format::R32UInt:
			{
				return DXGI_FORMAT_R32_UINT;
			}

			case Format::R32SInt:
			{
				return DXGI_FORMAT_R32_SINT;
			}

			case Format::RG8Unorm:
			{
				return DXGI_FORMAT_R8G8_UNORM;
			}

			case Format::RG8Snorm:
			{
				return DXGI_FORMAT_R8G8_SNORM;
			}

			case Format::RG8UInt:
			{
				return DXGI_FORMAT_R8G8_UINT;
			}

			case Format::RG8SInt:
			{
				return DXGI_FORMAT_R8G8_SINT;
			}

			case Format::RG16Unorm:
			{
				return DXGI_FORMAT_R16G16_UNORM;
			}

			case Format::RG16Snorm:
			{
				return DXGI_FORMAT_R16G16_SNORM;
			}

			case Format::RG16UInt:
			{
				return DXGI_FORMAT_R16G16_UINT;
			}

			case Format::RG16SInt:
			{
				return DXGI_FORMAT_R16G16_SINT;
			}

			case Format::RG16Float:
			{
				return DXGI_FORMAT_R16G16_FLOAT;
			}

			case Format::RG32Float:
			{
				return DXGI_FORMAT_R32G32_FLOAT;
			}

			case Format::RG32UInt:
			{
				return DXGI_FORMAT_R32G32_UINT;
			}

			case Format::RG32SInt:
			{
				return DXGI_FORMAT_R32G32_SINT;
			}

			case Format::RGB32Float:
			{
				return DXGI_FORMAT_R32G32B32_FLOAT;
			}

			case Format::RGBA16Float:
			{
				return DXGI_FORMAT_R16G16B16A16_FLOAT;
			}

			case Format::RGBA16Unorm:
			{
				return DXGI_FORMAT_R16G16B16A16_UNORM;
			}

			case Format::RGBA16Snorm:
			{
				return DXGI_FORMAT_R16G16B16A16_SNORM;
			}

			case Format::RGBA16UInt:
			{
				return DXGI_FORMAT_R16G16B16A16_UINT;
			}

			case Format::RGBA16SInt:
			{
				return DXGI_FORMAT_R16G16B16A16_SINT;
			}

			case Format::RGBA32Float:
			{
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			case Format::RGBA32UInt:
			{
				return DXGI_FORMAT_R32G32B32A32_UINT;
			}

			case Format::RGBA32SInt:
			{
				return DXGI_FORMAT_R32G32B32A32_SINT;
			}

			case Format::RGBA8:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			}

			case Format::RGBA8Snorm:
			{
				return DXGI_FORMAT_R8G8B8A8_SNORM;
			}

			case Format::RGBA8UInt:
			{
				return DXGI_FORMAT_R8G8B8A8_UINT;
			}

			case Format::RGBA8SInt:
			{
				return DXGI_FORMAT_R8G8B8A8_SINT;
			}

			case Format::RGBA8Srgb:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			}

			case Format::BGRA8:
			{
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			case Format::BGRA8Srgb:
			{
				return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			}

			case Format::RGB10A2Unorm:
			{
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			}

			case Format::RGB10A2UInt:
			{
				return DXGI_FORMAT_R10G10B10A2_UINT;
			}

			case Format::RG11B10Float:
			{
				return DXGI_FORMAT_R11G11B10_FLOAT;
			}

			case Format::RGB9E5Float:
			{
				return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
			}

			case Format::Depth16Unorm:
			{
				return DXGI_FORMAT_D16_UNORM;
			}

			case Format::Depth24Stencil8:
			{
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			}

			case Format::Depth32Float:
			{
				return DXGI_FORMAT_D32_FLOAT;
			}

			case Format::Depth32FloatStencil8:
			{
				return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			}

			case Format::BC1RGBAUnorm:
			{
				return DXGI_FORMAT_BC1_UNORM;
			}

			case Format::BC1RGBASrgb:
			{
				return DXGI_FORMAT_BC1_UNORM_SRGB;
			}

			case Format::BC2RGBAUnorm:
			{
				return DXGI_FORMAT_BC2_UNORM;
			}

			case Format::BC2RGBASrgb:
			{
				return DXGI_FORMAT_BC2_UNORM_SRGB;
			}

			case Format::BC3RGBAUnorm:
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			case Format::BC3RGBASrgb:
			{
				return DXGI_FORMAT_BC3_UNORM_SRGB;
			}

			case Format::BC4RUnorm:
			{
				return DXGI_FORMAT_BC4_UNORM;
			}

			case Format::BC4RSnorm:
			{
				return DXGI_FORMAT_BC4_SNORM;
			}

			case Format::BC5RGUnorm:
			{
				return DXGI_FORMAT_BC5_UNORM;
			}

			case Format::BC5RGSnorm:
			{
				return DXGI_FORMAT_BC5_SNORM;
			}

			case Format::BC6HRGBUFloat:
			{
				return DXGI_FORMAT_BC6H_UF16;
			}

			case Format::BC6HRGBSFloat:
			{
				return DXGI_FORMAT_BC6H_SF16;
			}

			case Format::BC7RGBAUnorm:
			{
				return DXGI_FORMAT_BC7_UNORM;
			}

			case Format::BC7RGBASrgb:
			{
				return DXGI_FORMAT_BC7_UNORM_SRGB;
			}

			default:
			{
				return DXGI_FORMAT_UNKNOWN;
			}
		}
	}

	inline DXGI_FORMAT nativeSwapChainFormat(
		Format format)
	{
		switch (format)
		{
			case Format::RGBA8Srgb:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			}

			case Format::BGRA8Srgb:
			{
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			default:
			{
				return nativeFormat(format);
			}
		}
	}

	inline DXGI_FORMAT nativeIndexFormat(
		IndexFormat format)
	{
		switch (format)
		{
			case IndexFormat::UInt16:
			{
				return DXGI_FORMAT_R16_UINT;
			}

			case IndexFormat::UInt32:
			default:
			{
				return DXGI_FORMAT_R32_UINT;
			}
		}
	}
} // namespace spall::dxgi
