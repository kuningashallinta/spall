#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Resources/Buffer/BufferInfo.h>
#include <spall/Resources/Texture/TextureInfo.h>

#include <cstdint>

namespace spall
{
	inline Status validateBufferResourceState(
		const BufferInfo& info,
		ResourceStateFlags state);

	inline Status validateTextureResourceState(
		const TextureInfo& info,
		ResourceStateFlags state,
		bool presentable);
} // namespace spall

#include <src/Validation/Common/ResourceStateValidation.inl>
