#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Resources/Buffer/BufferCreateInfo.h>
#include <src/Validation/Common/FlagValidation.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>

namespace spall
{
	inline Status validateBufferUsage(const BufferCreateInfo& info);

	inline Status validateBufferInitialState(const BufferCreateInfo& info);

	inline Status validateBufferCreateInfo(const BufferCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/BufferValidation.inl>
