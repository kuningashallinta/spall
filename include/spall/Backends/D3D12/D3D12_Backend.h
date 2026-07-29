#pragma once

#include <spall/Backend/IBackend.h>

namespace spall::d3d12
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
} // namespace spall::d3d12
