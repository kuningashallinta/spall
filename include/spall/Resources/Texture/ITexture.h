#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Resources/Texture/TextureInfo.h>

namespace spall
{
	/// Represents a two-dimensional GPU texture.
	class ITexture : public IResource
	{
	public:
		virtual TextureInfo info(void) const = 0;
	};
} // namespace spall
