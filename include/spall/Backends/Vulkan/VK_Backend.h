#pragma once

#include <spall/Backend/IBackend.h>

namespace spall::vk
{
	class Backend final : public IBackend
	{
	public:
		using IBackend::createDevice;

		RenderBackendType backendType(void) const override;

		Status createDevice(
			const DeviceCreateInfo& info,
			Resource<IDevice>* device) override;
	};
} // namespace spall::vk
