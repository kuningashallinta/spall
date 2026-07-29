#pragma once

#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

namespace spall
{
	class ITexture;

	/// Describes the aspects and mip range exposed by a texture view.
	struct TextureViewCreateInfo
	{
		ITexture* Texture = nullptr;

		/// Unknown inherits the texture format. Format reinterpretation is not supported.
		spall::Format Format = spall::Format::Unknown;

		TextureAspectFlags Aspects = TextureAspectFlags::None;

		std::uint32_t BaseMipLevel = 0;

		/// Zero includes every mip level from BaseMipLevel onward.
		std::uint32_t MipLevels = 0;

		std::uint32_t BaseArrayLayer = 0;
		std::uint32_t ArrayLayers = 0;

		bool Cubemap = false;
	};
} // namespace spall
