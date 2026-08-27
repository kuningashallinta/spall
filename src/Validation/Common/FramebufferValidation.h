#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Status/Status.h>
#include <spall/Framebuffer/FramebufferCreateInfo.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <src/Validation/Common/FormatValidation.h>
#include <src/Validation/Common/TextureValidation.h>

#include <cstdint>

namespace spall
{
	inline std::uint32_t framebufferSampleCount(const FramebufferCreateInfo& info);

	inline Status validateFramebufferSampleCounts(
		const FramebufferCreateInfo& info,
		std::uint32_t width,
		std::uint32_t height);

	inline Status validateFramebufferCreateInfo(const FramebufferCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/FramebufferValidation.inl>
