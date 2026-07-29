#pragma once

#include <catch2/catch_test_macros.hpp>

#include <spall/Common/Resource/Resource.h>
#include <spall/Device/IDevice.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <memory>
#include <utility>

namespace spall::tests
{
	struct RayTracingTestDevice
	{
		Resource<IDevice> Device;

		/// Null when the debug layer is unavailable.
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> InfoQueue;
	};

	/// Fails the current test if the debug layer recorded an error.
	///
	/// A missing barrier or a malformed acceleration-structure view is reported
	/// there rather than through a failed Status, so the check turns debug output
	/// into an assertion.
	inline void checkNoDeviceErrors(
		const RayTracingTestDevice& testDevice)
	{
		if (not testDevice.InfoQueue)
		{
			return;
		}

		CHECK(testDevice.InfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter() == 0);

		testDevice.InfoQueue->ClearStoredMessages();
	}

	/// Builds a D3D12 device on the first adapter that supports inline ray tracing,
	/// falling back to WARP.
	///
	/// The public backend deliberately never selects a software adapter, so this
	/// reaches past it. Everything but adapter selection is the ordinary device
	/// path, and WARP implements raytracing tier 1.1, which keeps these tests
	/// running on machines and CI runners without ray-tracing hardware.
	inline RayTracingTestDevice requireRayTracingDevice(
		void)
	{
		using Microsoft::WRL::ComPtr;

		UINT factoryFlags = 0;

		ComPtr<ID3D12Debug> debugController;

		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		}

		ComPtr<IDXGIFactory4> factory;

		if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory))))
		{
			SKIP("The DXGI factory is unavailable.");
		}

		ComPtr<ID3D12Device> hardwareDevice;

		for (UINT adapterIndex = 0;; ++adapterIndex)
		{
			ComPtr<IDXGIAdapter1> candidate;

			if (factory->EnumAdapters1(adapterIndex, &candidate) == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			DXGI_ADAPTER_DESC1 adapterDesc = {};

			if (FAILED(candidate->GetDesc1(&adapterDesc)) or ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0))
			{
				continue;
			}

			ComPtr<ID3D12Device> created;

			if (FAILED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&created))))
			{
				continue;
			}

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};

			if (SUCCEEDED(created->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options))) and
				(options.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1))
			{
				hardwareDevice = std::move(created);
				break;
			}
		}

		if (not hardwareDevice)
		{
			ComPtr<IDXGIAdapter> warpAdapter;

			if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))) or
				FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&hardwareDevice))))
			{
				SKIP("No adapter on this machine supports inline ray tracing.");
			}
		}

		RayTracingTestDevice testDevice = {};

		if (SUCCEEDED(hardwareDevice.As(&testDevice.InfoQueue)))
		{
			// Only errors matter here; the layer is chatty about everything else.
			D3D12_INFO_QUEUE_FILTER filter = {};
			D3D12_MESSAGE_SEVERITY allowedSeverities[] = {
				D3D12_MESSAGE_SEVERITY_CORRUPTION,
				D3D12_MESSAGE_SEVERITY_ERROR};

			filter.AllowList.NumSeverities = _countof(allowedSeverities);
			filter.AllowList.pSeverityList = allowedSeverities;

			testDevice.InfoQueue->PushRetrievalFilter(&filter);
		}

		std::unique_ptr<spall::d3d12::Device> device = std::make_unique<spall::d3d12::Device>(
			std::move(factory),
			std::move(hardwareDevice));

		if (device->initialize() != spall::SUCCESS)
		{
			SKIP("The ray-tracing device failed to initialize.");
		}

		if (not device->limits().SupportsInlineRayTracing)
		{
			SKIP("No adapter on this machine supports inline ray tracing.");
		}

		testDevice.Device = Resource<IDevice>(device.release());

		return testDevice;
	}
} // namespace spall::tests
