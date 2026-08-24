// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <src/Backends/D3D12/SwapChain/D3D12_SwapChain.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common.h>

#include <memory>
#include <utility>

namespace spall::d3d12
{
	Status Device::createSwapChain(
		const SwapChainCreateInfo& info,
		Resource<ISwapChain>* swapChain)
	{
		if (swapChain == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateSwapChainCreateInfo(info));

		if ((info.Format != Format::RGBA8) and
			(info.Format != Format::RGBA8Srgb) and
			(info.Format != Format::BGRA8) and
			(info.Format != Format::BGRA8Srgb) and
			(info.Format != Format::RGBA16Float))
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		const bool composited = info.AlphaMode == AlphaMode::Premultiplied;
		bool allowTearing = false;

		if (info.PresentMode == PresentMode::Immediate)
		{
			if (composited)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			ComPtr<IDXGIFactory5> factory5;
			HRESULT hr = m_Factory.As(&factory5);

			if (FAILED(hr))
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			BOOL tearingSupported = FALSE;
			hr = factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&tearingSupported,
				sizeof(tearingSupported));

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			if (tearingSupported == FALSE)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			allowTearing = true;
		}

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = info.Width;
		swapChainDesc.Height = info.Height;
		swapChainDesc.Format = swapChainFormat(info.Format);
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = composited ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		const HWND window = static_cast<HWND>(info.Window.Value);
		ComPtr<IDXGISwapChain1> dxgiSwapChain;

		ComPtr<IDCompositionDevice> compositionDevice;
		ComPtr<IDCompositionTarget> compositionTarget;
		ComPtr<IDCompositionVisual> compositionVisual;

		HRESULT hr = S_OK;

		if (composited)
		{
			hr = m_Factory->CreateSwapChainForComposition(m_CommandQueue.Get(), &swapChainDesc, nullptr, &dxgiSwapChain);

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&compositionDevice));

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = compositionDevice->CreateTargetForHwnd(window, TRUE, &compositionTarget);

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = compositionDevice->CreateVisual(&compositionVisual);

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = compositionVisual->SetContent(dxgiSwapChain.Get());

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = compositionTarget->SetRoot(compositionVisual.Get());

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = compositionDevice->Commit();

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}
		}
		else
		{
			hr = m_Factory->CreateSwapChainForHwnd(m_CommandQueue.Get(), window, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain);

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}

			hr = m_Factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

			if (FAILED(hr))
			{
				return mapHResult(hr);
			}
		}

		ComPtr<IDXGISwapChain3> swapChain3;
		hr = dxgiSwapChain.As(&swapChain3);

		if (FAILED(hr))
		{
			return mapHResult(hr);
		}

		std::unique_ptr<SwapChain> resultSwapChain = std::make_unique<SwapChain>(
			*this,
			std::move(swapChain3),
			info.Width,
			info.Height,
			info.Format,
			info.PresentMode,
			swapChainDesc.BufferCount);

		if (composited)
		{
			resultSwapChain->m_CompositionDevice = std::move(compositionDevice);
			resultSwapChain->m_CompositionTarget = std::move(compositionTarget);
			resultSwapChain->m_CompositionVisual = std::move(compositionVisual);
		}

		SPALL_TRY(resultSwapChain->recreateBackBuffers());

		*swapChain = Resource<ISwapChain>(resultSwapChain.release());

		return {};
	}
} // namespace spall::d3d12
