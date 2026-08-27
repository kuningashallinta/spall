#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Status/Status.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/RenderPass/RenderPassBeginInfo.h>
#include <src/Validation/Common/FormatValidation.h>

#include <cmath>
#include <cstdint>

namespace spall
{
	inline Status validatePassBeginInfo(const RenderPassBeginInfo& beginInfo);
} // namespace spall

#include <src/Validation/Common/RenderPassValidation.inl>
