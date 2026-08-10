#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/FormatValidation.h>

static constexpr spall::Format EveryFormat[] = {
	spall::Format::Unknown,
	spall::Format::R8,
	spall::Format::R32Float,
	spall::Format::RG16Float,
	spall::Format::RG32Float,
	spall::Format::RGB32Float,
	spall::Format::RGBA16Float,
	spall::Format::RGBA32Float,
	spall::Format::RGBA8,
	spall::Format::RGBA8Srgb,
	spall::Format::BGRA8,
	spall::Format::BGRA8Srgb,
	spall::Format::Depth24Stencil8,
	spall::Format::Depth32Float,
	spall::Format::R8Snorm,
	spall::Format::R8UInt,
	spall::Format::R8SInt,
	spall::Format::R16Unorm,
	spall::Format::R16Snorm,
	spall::Format::R16UInt,
	spall::Format::R16SInt,
	spall::Format::R16Float,
	spall::Format::R32UInt,
	spall::Format::R32SInt,
	spall::Format::RG8Unorm,
	spall::Format::RG8Snorm,
	spall::Format::RG8UInt,
	spall::Format::RG8SInt,
	spall::Format::RG16Unorm,
	spall::Format::RG16Snorm,
	spall::Format::RG16UInt,
	spall::Format::RG16SInt,
	spall::Format::RG32UInt,
	spall::Format::RG32SInt,
	spall::Format::RGBA8Snorm,
	spall::Format::RGBA8UInt,
	spall::Format::RGBA8SInt,
	spall::Format::RGBA16Unorm,
	spall::Format::RGBA16Snorm,
	spall::Format::RGBA16UInt,
	spall::Format::RGBA16SInt,
	spall::Format::RGBA32UInt,
	spall::Format::RGBA32SInt,
	spall::Format::RGB10A2Unorm,
	spall::Format::RGB10A2UInt,
	spall::Format::RG11B10Float,
	spall::Format::RGB9E5Float,
	spall::Format::Depth16Unorm,
	spall::Format::Depth32FloatStencil8,
	spall::Format::BC1RGBAUnorm,
	spall::Format::BC1RGBASrgb,
	spall::Format::BC2RGBAUnorm,
	spall::Format::BC2RGBASrgb,
	spall::Format::BC3RGBAUnorm,
	spall::Format::BC3RGBASrgb,
	spall::Format::BC4RUnorm,
	spall::Format::BC4RSnorm,
	spall::Format::BC5RGUnorm,
	spall::Format::BC5RGSnorm,
	spall::Format::BC6HRGBUFloat,
	spall::Format::BC6HRGBSFloat,
	spall::Format::BC7RGBAUnorm,
	spall::Format::BC7RGBASrgb};

TEST_CASE(
	"Only the depth formats are depth formats",
	"[format]")
{
	CHECK(spall::isDepthFormat(spall::Format::Depth24Stencil8));
	CHECK(spall::isDepthFormat(spall::Format::Depth32Float));
	CHECK(spall::isDepthFormat(spall::Format::Depth16Unorm));
	CHECK(spall::isDepthFormat(spall::Format::Depth32FloatStencil8));

	CHECK_FALSE(spall::isDepthFormat(spall::Format::RGBA8));
	CHECK_FALSE(spall::isDepthFormat(spall::Format::R32Float));
	CHECK_FALSE(spall::isDepthFormat(spall::Format::Unknown));
	CHECK_FALSE(spall::isSrgbFormat(spall::Format::Unknown));
}

TEST_CASE(
	"A depth format is never a color format",
	"[format]")
{
	CHECK_FALSE(spall::isColorFormat(spall::Format::Depth24Stencil8));
	CHECK_FALSE(spall::isColorFormat(spall::Format::Depth32Float));
	CHECK_FALSE(spall::isColorFormat(spall::Format::Depth16Unorm));
	CHECK_FALSE(spall::isColorFormat(spall::Format::Depth32FloatStencil8));
}

TEST_CASE(
	"Only combined depth-stencil formats have a stencil aspect",
	"[format][stencil]")
{
	CHECK(spall::hasStencilAspect(spall::Format::Depth24Stencil8));
	CHECK(spall::hasStencilAspect(spall::Format::Depth32FloatStencil8));

	CHECK_FALSE(spall::hasStencilAspect(spall::Format::Depth16Unorm));
	CHECK_FALSE(spall::hasStencilAspect(spall::Format::Depth32Float));
	CHECK_FALSE(spall::hasStencilAspect(spall::Format::RGBA8));
}

TEST_CASE(
	"Unknown is no kind of format",
	"[format]")
{
	CHECK_FALSE(spall::isColorFormat(spall::Format::Unknown));
	CHECK_FALSE(spall::isDepthFormat(spall::Format::Unknown));
	CHECK_FALSE(spall::isTextureFormat(spall::Format::Unknown));
	CHECK_FALSE(spall::isVertexFormat(spall::Format::Unknown));
	CHECK(spall::formatBytesPerPixel(spall::Format::Unknown) == 0);
}

TEST_CASE(
	"RGB32Float is a vertex format only",
	"[format]")
{
	CHECK(spall::isVertexFormat(spall::Format::RGB32Float));

	CHECK_FALSE(spall::isColorFormat(spall::Format::RGB32Float));
	CHECK_FALSE(spall::isTextureFormat(spall::Format::RGB32Float));
	CHECK(spall::formatBytesPerPixel(spall::Format::RGB32Float) == 0);
}

TEST_CASE(
	"A depth format is never a vertex format",
	"[format]")
{
	CHECK_FALSE(spall::isVertexFormat(spall::Format::Depth24Stencil8));
	CHECK_FALSE(spall::isVertexFormat(spall::Format::Depth32Float));
	CHECK_FALSE(spall::isVertexFormat(spall::Format::Depth16Unorm));
	CHECK_FALSE(spall::isVertexFormat(spall::Format::Depth32FloatStencil8));
}

TEST_CASE(
	"A texture format is a color or a depth format",
	"[format]")
{
	for (const spall::Format format : EveryFormat)
	{
		CHECK(spall::isTextureFormat(format) == (spall::isColorFormat(format) or spall::isDepthFormat(format)));
	}
}

TEST_CASE(
	"Only color formats with native vertex decoding are usable as vertex formats",
	"[format]")
{
	for (const spall::Format format : EveryFormat)
	{
		if (spall::isColorFormat(format))
		{
			const bool expected = (not spall::isSrgbFormat(format)) and
				(not spall::isBlockCompressedFormat(format)) and
				(format != spall::Format::RGB9E5Float);
			CHECK(spall::isVertexFormat(format) == expected);
		}
	}
}

TEST_CASE(
	"The eight-bit RGBA formats have explicit sRGB variants",
	"[format]")
{
	CHECK(spall::isSrgbFormat(spall::Format::RGBA8Srgb));
	CHECK(spall::isSrgbFormat(spall::Format::BGRA8Srgb));

	CHECK_FALSE(spall::isSrgbFormat(spall::Format::RGBA8));
	CHECK_FALSE(spall::isSrgbFormat(spall::Format::BGRA8));
}

TEST_CASE(
	"Every texture format has a block size",
	"[format]")
{
	for (const spall::Format format : EveryFormat)
	{
		if (spall::isTextureFormat(format))
		{
			CHECK(spall::formatBytesPerBlock(format) != 0);
			CHECK(spall::formatBlockWidth(format) != 0);
			CHECK(spall::formatBlockHeight(format) != 0);
		}
	}
}

TEST_CASE(
	"Every uncompressed texture format has a pixel size",
	"[format]")
{
	for (const spall::Format format : EveryFormat)
	{
		if (spall::isTextureFormat(format) and (not spall::isBlockCompressedFormat(format)))
		{
			CHECK(spall::formatBytesPerPixel(format) != 0);
		}
	}
}

TEST_CASE(
	"Only texture formats have a pixel size",
	"[format]")
{
	for (const spall::Format format : EveryFormat)
	{
		if (spall::formatBytesPerPixel(format) != 0)
		{
			CHECK(spall::isTextureFormat(format));
		}
	}
}

TEST_CASE(
	"Pixel sizes match their channel widths",
	"[format]")
{
	const spall::Format oneByteFormats[] = {
		spall::Format::R8,
		spall::Format::R8Snorm,
		spall::Format::R8UInt,
		spall::Format::R8SInt};
	const spall::Format twoByteFormats[] = {
		spall::Format::R16Unorm,
		spall::Format::R16Snorm,
		spall::Format::R16UInt,
		spall::Format::R16SInt,
		spall::Format::R16Float,
		spall::Format::RG8Unorm,
		spall::Format::RG8Snorm,
		spall::Format::RG8UInt,
		spall::Format::RG8SInt};
	const spall::Format fourByteFormats[] = {
		spall::Format::R32Float,
		spall::Format::R32UInt,
		spall::Format::R32SInt,
		spall::Format::RG16Float,
		spall::Format::RG16Unorm,
		spall::Format::RG16Snorm,
		spall::Format::RG16UInt,
		spall::Format::RG16SInt,
		spall::Format::RGBA8,
		spall::Format::RGBA8Snorm,
		spall::Format::RGBA8UInt,
		spall::Format::RGBA8SInt,
		spall::Format::RGBA8Srgb,
		spall::Format::BGRA8,
		spall::Format::BGRA8Srgb,
		spall::Format::RGB10A2Unorm,
		spall::Format::RGB10A2UInt,
		spall::Format::RG11B10Float,
		spall::Format::RGB9E5Float};
	const spall::Format eightByteFormats[] = {
		spall::Format::RG32Float,
		spall::Format::RG32UInt,
		spall::Format::RG32SInt,
		spall::Format::RGBA16Float,
		spall::Format::RGBA16Unorm,
		spall::Format::RGBA16Snorm,
		spall::Format::RGBA16UInt,
		spall::Format::RGBA16SInt};
	const spall::Format sixteenByteFormats[] = {
		spall::Format::RGBA32Float,
		spall::Format::RGBA32UInt,
		spall::Format::RGBA32SInt};

	for (const spall::Format format : oneByteFormats)
	{
		CHECK(spall::formatBytesPerPixel(format) == 1);
	}

	for (const spall::Format format : twoByteFormats)
	{
		CHECK(spall::formatBytesPerPixel(format) == 2);
	}

	for (const spall::Format format : fourByteFormats)
	{
		CHECK(spall::formatBytesPerPixel(format) == 4);
	}

	for (const spall::Format format : eightByteFormats)
	{
		CHECK(spall::formatBytesPerPixel(format) == 8);
	}

	for (const spall::Format format : sixteenByteFormats)
	{
		CHECK(spall::formatBytesPerPixel(format) == 16);
	}
}

TEST_CASE(
	"Block-compressed formats are sampled color formats without a pixel size",
	"[format][compressed]")
{
	for (const spall::Format format : EveryFormat)
	{
		if (not spall::isBlockCompressedFormat(format))
		{
			continue;
		}

		CHECK(spall::isColorFormat(format));
		CHECK(spall::isTextureFormat(format));
		CHECK_FALSE(spall::isDepthFormat(format));
		CHECK_FALSE(spall::isVertexFormat(format));
		CHECK(spall::formatBytesPerPixel(format) == 0);
		CHECK(spall::formatBlockWidth(format) == 4);
		CHECK(spall::formatBlockHeight(format) == 4);
	}
}

TEST_CASE(
	"Block sizes match the block-compression scheme",
	"[format][compressed]")
{
	const spall::Format eightByteBlockFormats[] = {
		spall::Format::BC1RGBAUnorm,
		spall::Format::BC1RGBASrgb,
		spall::Format::BC4RUnorm,
		spall::Format::BC4RSnorm};
	const spall::Format sixteenByteBlockFormats[] = {
		spall::Format::BC2RGBAUnorm,
		spall::Format::BC2RGBASrgb,
		spall::Format::BC3RGBAUnorm,
		spall::Format::BC3RGBASrgb,
		spall::Format::BC5RGUnorm,
		spall::Format::BC5RGSnorm,
		spall::Format::BC6HRGBUFloat,
		spall::Format::BC6HRGBSFloat,
		spall::Format::BC7RGBAUnorm,
		spall::Format::BC7RGBASrgb};

	for (const spall::Format format : eightByteBlockFormats)
	{
		CHECK(spall::formatBytesPerBlock(format) == 8);
	}

	for (const spall::Format format : sixteenByteBlockFormats)
	{
		CHECK(spall::formatBytesPerBlock(format) == 16);
	}
}

TEST_CASE(
	"The block-compressed sRGB variants are sRGB formats",
	"[format][compressed][srgb]")
{
	CHECK(spall::isSrgbFormat(spall::Format::BC1RGBASrgb));
	CHECK(spall::isSrgbFormat(spall::Format::BC2RGBASrgb));
	CHECK(spall::isSrgbFormat(spall::Format::BC3RGBASrgb));
	CHECK(spall::isSrgbFormat(spall::Format::BC7RGBASrgb));

	CHECK_FALSE(spall::isSrgbFormat(spall::Format::BC1RGBAUnorm));
	CHECK_FALSE(spall::isSrgbFormat(spall::Format::BC4RUnorm));
	CHECK_FALSE(spall::isSrgbFormat(spall::Format::BC6HRGBUFloat));
}

TEST_CASE(
	"An uncompressed format is its own block",
	"[format][compressed]")
{
	CHECK(spall::formatBlockWidth(spall::Format::RGBA8) == 1);
	CHECK(spall::formatBlockHeight(spall::Format::RGBA8) == 1);
	CHECK(spall::formatBytesPerBlock(spall::Format::RGBA8) == spall::formatBytesPerPixel(spall::Format::RGBA8));
	CHECK(spall::formatBlockWidth(spall::Format::Unknown) == 0);
	CHECK(spall::formatBlockWidth(spall::Format::RGB32Float) == 0);
}

TEST_CASE(
	"A block count rounds up to whole blocks",
	"[format][compressed]")
{
	CHECK(spall::formatBlockCount(64, 4) == 16);
	CHECK(spall::formatBlockCount(5, 4) == 2);
	CHECK(spall::formatBlockCount(1, 4) == 1);
	CHECK(spall::formatBlockCount(0, 4) == 0);
	CHECK(spall::formatBlockCount(7, 1) == 7);
	CHECK(spall::formatBlockCount(7, 0) == 0);
}

TEST_CASE(
	"Depth format sizes match their native storage",
	"[format]")
{
	CHECK(spall::formatBytesPerPixel(spall::Format::Depth16Unorm) == 2);
	CHECK(spall::formatBytesPerPixel(spall::Format::Depth24Stencil8) == 4);
	CHECK(spall::formatBytesPerPixel(spall::Format::Depth32Float) == 4);
	CHECK(spall::formatBytesPerPixel(spall::Format::Depth32FloatStencil8) == 8);
}
