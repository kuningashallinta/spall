// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	/// Identifies the element or texel representation of a graphics resource.
	///
	/// Unorm and Snorm formats are normalized integers. Srgb formats perform
	/// transfer-function conversion when sampled or written for presentation.
	enum class Format
	{
		Unknown,
		R8,
		R32Float,
		RG16Float,
		RG32Float,
		RGB32Float,
		RGBA16Float,
		RGBA32Float,
		RGBA8,
		BGRA8,
		Depth24Stencil8,
		Depth32Float,
		RGBA8Srgb,
		BGRA8Srgb,

		R8Snorm,
		R8UInt,
		R8SInt,
		R16Unorm,
		R16Snorm,
		R16UInt,
		R16SInt,
		R16Float,
		R32UInt,
		R32SInt,

		RG8Unorm,
		RG8Snorm,
		RG8UInt,
		RG8SInt,
		RG16Unorm,
		RG16Snorm,
		RG16UInt,
		RG16SInt,
		RG32UInt,
		RG32SInt,

		RGBA8Snorm,
		RGBA8UInt,
		RGBA8SInt,
		RGBA16Unorm,
		RGBA16Snorm,
		RGBA16UInt,
		RGBA16SInt,
		RGBA32UInt,
		RGBA32SInt,

		RGB10A2Unorm,
		RGB10A2UInt,
		RG11B10Float,
		RGB9E5Float,

		Depth16Unorm,
		Depth32FloatStencil8,

		BC1RGBAUnorm,
		BC1RGBASrgb,
		BC2RGBAUnorm,
		BC2RGBASrgb,
		BC3RGBAUnorm,
		BC3RGBASrgb,
		BC4RUnorm,
		BC4RSnorm,
		BC5RGUnorm,
		BC5RGSnorm,
		BC6HRGBUFloat,
		BC6HRGBSFloat,
		BC7RGBAUnorm,
		BC7RGBASrgb
	};
} // namespace spall
