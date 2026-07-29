#pragma once

#include <spall/Common/Enums/Format.h>

#include <cstdint>

namespace spall
{
	inline bool isDepthFormat(
		Format format)
	{
		return (format == Format::Depth16Unorm) or
			(format == Format::Depth24Stencil8) or
			(format == Format::Depth32Float) or
			(format == Format::Depth32FloatStencil8);
	}

	inline bool hasStencilAspect(
		Format format)
	{
		return (format == Format::Depth24Stencil8) or
			(format == Format::Depth32FloatStencil8);
	}

	inline bool isBlockCompressedFormat(
		Format format);

	inline bool isColorFormat(
		Format format);

	inline bool isSrgbFormat(
		Format format)
	{
		return (format == Format::RGBA8Srgb) or (format == Format::BGRA8Srgb) or
			(format == Format::BC1RGBASrgb) or (format == Format::BC2RGBASrgb) or
			(format == Format::BC3RGBASrgb) or (format == Format::BC7RGBASrgb);
	}

	inline bool isTextureFormat(
		Format format)
	{
		return isColorFormat(format) or isDepthFormat(format);
	}

	inline bool isRenderTargetFormat(
		Format format)
	{
		return isColorFormat(format) and not isBlockCompressedFormat(format);
	}

	inline bool isVertexFormat(
		Format format)
	{
		return ((isColorFormat(format) and not isSrgbFormat(format) and not isBlockCompressedFormat(format) and
					(format != Format::RGB9E5Float)) or
			(format == Format::RGB32Float));
	}

	inline std::uint32_t formatBytesPerPixel(
		Format format);

	inline std::uint32_t formatBlockWidth(
		Format format);

	inline std::uint32_t formatBlockHeight(
		Format format)
	{
		return formatBlockWidth(format);
	}

	inline std::uint32_t formatBytesPerBlock(
		Format format);

	inline std::uint32_t formatBlockCount(
		std::uint32_t extent,
		std::uint32_t blockExtent);
} // namespace spall

#include <src/Validation/Common/FormatValidation.inl>
