// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <spall/Backends/D3D12/D3D12_Backend.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <memory>
#include <utility>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace spall::d3d12
{
	RenderBackendType Backend::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	Status Backend::createDevice(
		const DeviceCreateInfo& info,
		Resource<IDevice>* device)
	{
		if (device == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		UINT factoryFlags = 0;

		if (info.Debug)
		{
			ComPtr<ID3D12Debug> debugController;
			const HRESULT debugResult = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));

			if (FAILED(debugResult))
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			debugController->EnableDebugLayer();
			factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		}

		ComPtr<IDXGIFactory4> factory;
		HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		ComPtr<IDXGIFactory6> factory6;
		const bool orderedByPreference = SUCCEEDED(factory.As(&factory6));

		ComPtr<ID3D12Device> d3dDevice;

		for (UINT adapterIndex = 0;; ++adapterIndex)
		{
			ComPtr<IDXGIAdapter1> candidate;

			hr = orderedByPreference
				? factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidate))
				: factory->EnumAdapters1(adapterIndex, &candidate);

			if (hr == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			if (FAILED(hr))
			{
				return mapStatus(hr);
			}

			DXGI_ADAPTER_DESC1 adapterDesc = {};
			hr = candidate->GetDesc1(&adapterDesc);

			if (FAILED(hr) or ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0))
			{
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice))))
			{
				break;
			}
		}

		if (not d3dDevice)
		{
			return ERR_UNSUPPORTED_BACKEND;
		}

		std::unique_ptr<Device> createdDevice = std::make_unique<Device>(
			std::move(factory),
			std::move(d3dDevice));

		SPALL_TRY(createdDevice->initialize());

		*device = Resource<IDevice>(createdDevice.release());

		return {};
	}
} // namespace spall::d3d12
