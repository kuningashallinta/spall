#pragma once

#include <spall/Common/Enums/Format.h>
#include <spall/Resources/Texture/TextureRegion.h>
#include <src/Validation/Common/FormatValidation.h>

#include <cstdint>

namespace spall::dxgi
{
	/// Describes the tightly packed rows a texture region occupies in a buffer.
	struct RegionLayout
	{
		std::uint32_t RowBytes = 0;
		std::uint32_t RowCount = 0;

		/// Region extent rounded up to whole compression blocks, as a copy box requires.
		std::uint32_t FootprintWidth = 0;
		std::uint32_t FootprintHeight = 0;
	};

	inline RegionLayout regionLayout(
		Format format,
		const TextureRegion& region);
} // namespace spall::dxgi

#include <src/Common/DXGI/DXGICopyLayout.inl>
