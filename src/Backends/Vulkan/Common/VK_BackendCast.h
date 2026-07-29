#pragma once

#include <spall/Common/Enums/RenderBackendType.h>

namespace spall::vk
{
	template <typename BackendType, typename InterfaceType>
	BackendType* backendCast(
		InterfaceType& object)
	{
		return (object.backendType() == RenderBackendType::Vulkan) ? static_cast<BackendType*>(&object) : nullptr;
	}

	template <typename BackendType, typename InterfaceType>
	BackendType* backendCast(
		InterfaceType* object)
	{
		return (object != nullptr) ? backendCast<BackendType>(*object) : nullptr;
	}
} // namespace spall::vk
