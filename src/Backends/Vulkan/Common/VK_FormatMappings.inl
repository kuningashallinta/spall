// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline std::optional<VkFormat> toVkFormat(
		Format format)
	{
		switch (format)
		{
			case Format::R8:
			{
				return VK_FORMAT_R8_UNORM;
			}

			case Format::R8Snorm:
			{
				return VK_FORMAT_R8_SNORM;
			}

			case Format::R8UInt:
			{
				return VK_FORMAT_R8_UINT;
			}

			case Format::R8SInt:
			{
				return VK_FORMAT_R8_SINT;
			}

			case Format::R16Unorm:
			{
				return VK_FORMAT_R16_UNORM;
			}

			case Format::R16Snorm:
			{
				return VK_FORMAT_R16_SNORM;
			}

			case Format::R16UInt:
			{
				return VK_FORMAT_R16_UINT;
			}

			case Format::R16SInt:
			{
				return VK_FORMAT_R16_SINT;
			}

			case Format::R16Float:
			{
				return VK_FORMAT_R16_SFLOAT;
			}

			case Format::R32Float:
			{
				return VK_FORMAT_R32_SFLOAT;
			}

			case Format::R32UInt:
			{
				return VK_FORMAT_R32_UINT;
			}

			case Format::R32SInt:
			{
				return VK_FORMAT_R32_SINT;
			}

			case Format::RG8Unorm:
			{
				return VK_FORMAT_R8G8_UNORM;
			}

			case Format::RG8Snorm:
			{
				return VK_FORMAT_R8G8_SNORM;
			}

			case Format::RG8UInt:
			{
				return VK_FORMAT_R8G8_UINT;
			}

			case Format::RG8SInt:
			{
				return VK_FORMAT_R8G8_SINT;
			}

			case Format::RG16Unorm:
			{
				return VK_FORMAT_R16G16_UNORM;
			}

			case Format::RG16Snorm:
			{
				return VK_FORMAT_R16G16_SNORM;
			}

			case Format::RG16UInt:
			{
				return VK_FORMAT_R16G16_UINT;
			}

			case Format::RG16SInt:
			{
				return VK_FORMAT_R16G16_SINT;
			}

			case Format::RG16Float:
			{
				return VK_FORMAT_R16G16_SFLOAT;
			}

			case Format::RG32Float:
			{
				return VK_FORMAT_R32G32_SFLOAT;
			}

			case Format::RG32UInt:
			{
				return VK_FORMAT_R32G32_UINT;
			}

			case Format::RG32SInt:
			{
				return VK_FORMAT_R32G32_SINT;
			}

			case Format::RGB32Float:
			{
				return VK_FORMAT_R32G32B32_SFLOAT;
			}

			case Format::RGBA16Float:
			{
				return VK_FORMAT_R16G16B16A16_SFLOAT;
			}

			case Format::RGBA16Unorm:
			{
				return VK_FORMAT_R16G16B16A16_UNORM;
			}

			case Format::RGBA16Snorm:
			{
				return VK_FORMAT_R16G16B16A16_SNORM;
			}

			case Format::RGBA16UInt:
			{
				return VK_FORMAT_R16G16B16A16_UINT;
			}

			case Format::RGBA16SInt:
			{
				return VK_FORMAT_R16G16B16A16_SINT;
			}

			case Format::RGBA32Float:
			{
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			}

			case Format::RGBA32UInt:
			{
				return VK_FORMAT_R32G32B32A32_UINT;
			}

			case Format::RGBA32SInt:
			{
				return VK_FORMAT_R32G32B32A32_SINT;
			}

			case Format::RGBA8:
			{
				return VK_FORMAT_R8G8B8A8_UNORM;
			}

			case Format::RGBA8Snorm:
			{
				return VK_FORMAT_R8G8B8A8_SNORM;
			}

			case Format::RGBA8UInt:
			{
				return VK_FORMAT_R8G8B8A8_UINT;
			}

			case Format::RGBA8SInt:
			{
				return VK_FORMAT_R8G8B8A8_SINT;
			}

			case Format::RGBA8Srgb:
			{
				return VK_FORMAT_R8G8B8A8_SRGB;
			}

			case Format::BGRA8:
			{
				return VK_FORMAT_B8G8R8A8_UNORM;
			}

			case Format::BGRA8Srgb:
			{
				return VK_FORMAT_B8G8R8A8_SRGB;
			}

			case Format::RGB10A2Unorm:
			{
				return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
			}

			case Format::RGB10A2UInt:
			{
				return VK_FORMAT_A2B10G10R10_UINT_PACK32;
			}

			case Format::RG11B10Float:
			{
				return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
			}

			case Format::RGB9E5Float:
			{
				return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
			}

			case Format::Depth16Unorm:
			{
				return VK_FORMAT_D16_UNORM;
			}

			case Format::Depth24Stencil8:
			{
				return VK_FORMAT_D24_UNORM_S8_UINT;
			}

			case Format::Depth32Float:
			{
				return VK_FORMAT_D32_SFLOAT;
			}

			case Format::Depth32FloatStencil8:
			{
				return VK_FORMAT_D32_SFLOAT_S8_UINT;
			}

			case Format::BC1RGBAUnorm:
			{
				return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			}

			case Format::BC1RGBASrgb:
			{
				return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			}

			case Format::BC2RGBAUnorm:
			{
				return VK_FORMAT_BC2_UNORM_BLOCK;
			}

			case Format::BC2RGBASrgb:
			{
				return VK_FORMAT_BC2_SRGB_BLOCK;
			}

			case Format::BC3RGBAUnorm:
			{
				return VK_FORMAT_BC3_UNORM_BLOCK;
			}

			case Format::BC3RGBASrgb:
			{
				return VK_FORMAT_BC3_SRGB_BLOCK;
			}

			case Format::BC4RUnorm:
			{
				return VK_FORMAT_BC4_UNORM_BLOCK;
			}

			case Format::BC4RSnorm:
			{
				return VK_FORMAT_BC4_SNORM_BLOCK;
			}

			case Format::BC5RGUnorm:
			{
				return VK_FORMAT_BC5_UNORM_BLOCK;
			}

			case Format::BC5RGSnorm:
			{
				return VK_FORMAT_BC5_SNORM_BLOCK;
			}

			case Format::BC6HRGBUFloat:
			{
				return VK_FORMAT_BC6H_UFLOAT_BLOCK;
			}

			case Format::BC6HRGBSFloat:
			{
				return VK_FORMAT_BC6H_SFLOAT_BLOCK;
			}

			case Format::BC7RGBAUnorm:
			{
				return VK_FORMAT_BC7_UNORM_BLOCK;
			}

			case Format::BC7RGBASrgb:
			{
				return VK_FORMAT_BC7_SRGB_BLOCK;
			}

			default:
			{
				return std::nullopt;
			}
		}
	}

	inline std::optional<Format> toSpallFormat(
		VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_R8_UNORM:
			{
				return Format::R8;
			}

			case VK_FORMAT_R8_SNORM:
			{
				return Format::R8Snorm;
			}

			case VK_FORMAT_R8_UINT:
			{
				return Format::R8UInt;
			}

			case VK_FORMAT_R8_SINT:
			{
				return Format::R8SInt;
			}

			case VK_FORMAT_R16_UNORM:
			{
				return Format::R16Unorm;
			}

			case VK_FORMAT_R16_SNORM:
			{
				return Format::R16Snorm;
			}

			case VK_FORMAT_R16_UINT:
			{
				return Format::R16UInt;
			}

			case VK_FORMAT_R16_SINT:
			{
				return Format::R16SInt;
			}

			case VK_FORMAT_R16_SFLOAT:
			{
				return Format::R16Float;
			}

			case VK_FORMAT_R32_SFLOAT:
			{
				return Format::R32Float;
			}

			case VK_FORMAT_R32_UINT:
			{
				return Format::R32UInt;
			}

			case VK_FORMAT_R32_SINT:
			{
				return Format::R32SInt;
			}

			case VK_FORMAT_R8G8_UNORM:
			{
				return Format::RG8Unorm;
			}

			case VK_FORMAT_R8G8_SNORM:
			{
				return Format::RG8Snorm;
			}

			case VK_FORMAT_R8G8_UINT:
			{
				return Format::RG8UInt;
			}

			case VK_FORMAT_R8G8_SINT:
			{
				return Format::RG8SInt;
			}

			case VK_FORMAT_R16G16_UNORM:
			{
				return Format::RG16Unorm;
			}

			case VK_FORMAT_R16G16_SNORM:
			{
				return Format::RG16Snorm;
			}

			case VK_FORMAT_R16G16_UINT:
			{
				return Format::RG16UInt;
			}

			case VK_FORMAT_R16G16_SINT:
			{
				return Format::RG16SInt;
			}

			case VK_FORMAT_R16G16_SFLOAT:
			{
				return Format::RG16Float;
			}

			case VK_FORMAT_R32G32_SFLOAT:
			{
				return Format::RG32Float;
			}

			case VK_FORMAT_R32G32_UINT:
			{
				return Format::RG32UInt;
			}

			case VK_FORMAT_R32G32_SINT:
			{
				return Format::RG32SInt;
			}

			case VK_FORMAT_R32G32B32_SFLOAT:
			{
				return Format::RGB32Float;
			}

			case VK_FORMAT_R16G16B16A16_SFLOAT:
			{
				return Format::RGBA16Float;
			}

			case VK_FORMAT_R16G16B16A16_UNORM:
			{
				return Format::RGBA16Unorm;
			}

			case VK_FORMAT_R16G16B16A16_SNORM:
			{
				return Format::RGBA16Snorm;
			}

			case VK_FORMAT_R16G16B16A16_UINT:
			{
				return Format::RGBA16UInt;
			}

			case VK_FORMAT_R16G16B16A16_SINT:
			{
				return Format::RGBA16SInt;
			}

			case VK_FORMAT_R32G32B32A32_SFLOAT:
			{
				return Format::RGBA32Float;
			}

			case VK_FORMAT_R32G32B32A32_UINT:
			{
				return Format::RGBA32UInt;
			}

			case VK_FORMAT_R32G32B32A32_SINT:
			{
				return Format::RGBA32SInt;
			}

			case VK_FORMAT_R8G8B8A8_UNORM:
			{
				return Format::RGBA8;
			}

			case VK_FORMAT_R8G8B8A8_SNORM:
			{
				return Format::RGBA8Snorm;
			}

			case VK_FORMAT_R8G8B8A8_UINT:
			{
				return Format::RGBA8UInt;
			}

			case VK_FORMAT_R8G8B8A8_SINT:
			{
				return Format::RGBA8SInt;
			}

			case VK_FORMAT_R8G8B8A8_SRGB:
			{
				return Format::RGBA8Srgb;
			}

			case VK_FORMAT_B8G8R8A8_UNORM:
			{
				return Format::BGRA8;
			}

			case VK_FORMAT_B8G8R8A8_SRGB:
			{
				return Format::BGRA8Srgb;
			}

			case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			{
				return Format::RGB10A2Unorm;
			}

			case VK_FORMAT_A2B10G10R10_UINT_PACK32:
			{
				return Format::RGB10A2UInt;
			}

			case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			{
				return Format::RG11B10Float;
			}

			case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			{
				return Format::RGB9E5Float;
			}

			case VK_FORMAT_D16_UNORM:
			{
				return Format::Depth16Unorm;
			}

			case VK_FORMAT_D24_UNORM_S8_UINT:
			{
				return Format::Depth24Stencil8;
			}

			case VK_FORMAT_D32_SFLOAT:
			{
				return Format::Depth32Float;
			}

			case VK_FORMAT_D32_SFLOAT_S8_UINT:
			{
				return Format::Depth32FloatStencil8;
			}

			case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			{
				return Format::BC1RGBAUnorm;
			}

			case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			{
				return Format::BC1RGBASrgb;
			}

			case VK_FORMAT_BC2_UNORM_BLOCK:
			{
				return Format::BC2RGBAUnorm;
			}

			case VK_FORMAT_BC2_SRGB_BLOCK:
			{
				return Format::BC2RGBASrgb;
			}

			case VK_FORMAT_BC3_UNORM_BLOCK:
			{
				return Format::BC3RGBAUnorm;
			}

			case VK_FORMAT_BC3_SRGB_BLOCK:
			{
				return Format::BC3RGBASrgb;
			}

			case VK_FORMAT_BC4_UNORM_BLOCK:
			{
				return Format::BC4RUnorm;
			}

			case VK_FORMAT_BC4_SNORM_BLOCK:
			{
				return Format::BC4RSnorm;
			}

			case VK_FORMAT_BC5_UNORM_BLOCK:
			{
				return Format::BC5RGUnorm;
			}

			case VK_FORMAT_BC5_SNORM_BLOCK:
			{
				return Format::BC5RGSnorm;
			}

			case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			{
				return Format::BC6HRGBUFloat;
			}

			case VK_FORMAT_BC6H_SFLOAT_BLOCK:
			{
				return Format::BC6HRGBSFloat;
			}

			case VK_FORMAT_BC7_UNORM_BLOCK:
			{
				return Format::BC7RGBAUnorm;
			}

			case VK_FORMAT_BC7_SRGB_BLOCK:
			{
				return Format::BC7RGBASrgb;
			}

			default:
			{
				return std::nullopt;
			}
		}
	}

	inline FormatCapabilities formatCapabilities(
		Format format,
		const VkFormatProperties& properties)
	{
		FormatCapabilities capabilities = {};

		if (isVertexFormat(format) and
			((properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) != 0))
		{
			capabilities.SupportsVertexInput = true;
		}

		if (not isTextureFormat(format))
		{
			return capabilities;
		}

		const VkFormatFeatureFlags features = properties.optimalTilingFeatures;

		if ((features & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT) != 0)
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::TransferSource;
		}

		if ((features & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) != 0)
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::TransferDestination;
		}

		if (isColorFormat(format) and ((features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::ColorAttachment;
			capabilities.SupportsBlending = ((features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) != 0);
		}

		if (isDepthFormat(format) and ((features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::DepthStencilAttachment;
		}

		if (not isDepthFormat(format) and ((features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::Sampled;
			capabilities.SupportsLinearFiltering = ((features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0);
		}

		if (not isDepthFormat(format) and ((features & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::Storage;
		}

		return capabilities;
	}

	inline VkImageAspectFlags aspectMask(
		TextureAspectFlags aspects)
	{
		VkImageAspectFlags aspectMask = 0;

		if ((aspects & TextureAspectFlags::Color) != TextureAspectFlags::None)
		{
			aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if ((aspects & TextureAspectFlags::Depth) != TextureAspectFlags::None)
		{
			aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if ((aspects & TextureAspectFlags::Stencil) != TextureAspectFlags::None)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		return aspectMask;
	}

	inline TextureAspectFlags defaultAspects(
		Format format)
	{
		if (isBlockCompressedFormat(format))
		{
			return TextureAspectFlags::Color;
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
				return TextureAspectFlags::Color;
			}

			case Format::Depth24Stencil8:
			case Format::Depth32FloatStencil8:
			{
				return TextureAspectFlags::Depth | TextureAspectFlags::Stencil;
			}

			case Format::Depth16Unorm:
			case Format::Depth32Float:
			{
				return TextureAspectFlags::Depth;
			}

			default:
			{
				return TextureAspectFlags::None;
			}
		}
	}

	inline std::optional<TextureFormatProperties> textureFormatInfo(
		Format format)
	{
		if (not isColorFormat(format))
		{
			return std::nullopt;
		}

		return TextureFormatProperties {formatBytesPerBlock(format), formatBlockWidth(format), formatBlockHeight(format)};
	}
} // namespace spall::vk
