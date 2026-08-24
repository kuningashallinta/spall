// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Queue/D3D12_FenceTimeline.h>

#include <spall/Common/Assert.h>
#include <src/Common/DXGI/DXGIError.h>

#include <limits>

namespace spall::d3d12
{
	FenceTimeline::~FenceTimeline()
	{
		if (m_FenceEvent != nullptr)
		{
			CloseHandle(m_FenceEvent);
		}
	}

	Status FenceTimeline::initialize(
		ID3D12Device& device)
	{
		const HRESULT hr = device.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

		if (m_FenceEvent == nullptr)
		{
			return ERR_BACKEND_FAILURE;
		}

		return {};
	}

	std::uint64_t FenceTimeline::completedFenceValue() const
	{
		return m_Fence ? m_Fence->GetCompletedValue() : 0;
	}

	Status FenceTimeline::signal(
		ID3D12CommandQueue& queue,
		std::uint64_t* fenceValue)
	{
		SPALL_ASSERT(fenceValue != nullptr);
		SPALL_ASSERT(m_NextFenceValue < (std::numeric_limits<std::uint64_t>::max)());

		const std::uint64_t value = m_NextFenceValue;
		const HRESULT hr = queue.Signal(m_Fence.Get(), value);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		++m_NextFenceValue;
		*fenceValue = value;

		return {};
	}

	Status FenceTimeline::waitForFenceValue(
		std::uint64_t fenceValue)
	{
		if ((fenceValue == 0) or (not m_Fence) or (m_Fence->GetCompletedValue() >= fenceValue))
		{
			return {};
		}

		const HRESULT hr = m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		if (WaitForSingleObject(m_FenceEvent, INFINITE) != WAIT_OBJECT_0)
		{
			return ERR_BACKEND_FAILURE;
		}

		return {};
	}

	ID3D12Fence* FenceTimeline::fence() const
	{
		return m_Fence.Get();
	}

	std::uint64_t FenceTimeline::lastSignaledValue() const
	{
		return m_NextFenceValue - 1;
	}
} // namespace spall::d3d12
