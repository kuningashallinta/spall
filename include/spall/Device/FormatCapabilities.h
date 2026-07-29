#pragma once

#include <spall/Common/Enums/TextureUsageFlags.h>

namespace spall
{
	/// Describes how a format can be used by a graphics device.
	///
	/// The reported capabilities include only features exposed through Spall RHI's
	/// portable API.
	struct FormatCapabilities
	{
		/// Texture usages supported by the format.
		TextureUsageFlags SupportedTextureUsages = TextureUsageFlags::None;

		/// Indicates whether the format can describe a vertex attribute.
		bool SupportsVertexInput = false;

		/// Indicates whether sampled textures support linear filtering.
		///
		/// This value is meaningful only when TextureUsageFlags::Sampled is
		/// included in SupportedTextureUsages.
		bool SupportsLinearFiltering = false;

		/// Indicates whether color attachments support blending.
		///
		/// This value is meaningful only when TextureUsageFlags::ColorAttachment
		/// is included in SupportedTextureUsages.
		bool SupportsBlending = false;
	};
} // namespace spall
