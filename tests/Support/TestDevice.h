#pragma once

#include <catch2/catch_test_macros.hpp>

#include <spall/Backend/BackendFactory.h>
#include <spall/Backend/IBackend.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Device/IDevice.h>

#include <memory>

struct TestDevice
{
	std::unique_ptr<spall::IBackend> Backend;
	spall::Resource<spall::IDevice> Device;
};

static TestDevice requireDevice(
	spall::RenderBackendType backendType)
{
	TestDevice testDevice = {};

	if (spall::createBackend(backendType, &testDevice.Backend) != spall::SUCCESS or (not testDevice.Backend))
	{
		SKIP("The requested backend is unavailable.");
	}

	spall::DeviceCreateInfo deviceInfo = {};
	deviceInfo.Debug = true;

	if (testDevice.Backend->createDevice(deviceInfo, &testDevice.Device) != spall::SUCCESS)
	{
		deviceInfo.Debug = false;

		if (testDevice.Backend->createDevice(deviceInfo, &testDevice.Device) != spall::SUCCESS)
		{
			SKIP("No adapter on this machine supports the requested backend.");
		}
	}

	return testDevice;
}
