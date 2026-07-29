#pragma once

#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>

namespace spall::d3d12
{
	/// A monotonic fence and its CPU wait event, shared by every queue kind.
	class FenceTimeline
	{
	public:
		~FenceTimeline(void);

		Status initialize(ID3D12Device& device);

		/// Signals the fence on the queue and reports the value marking the submitted work.
		Status signal(
			ID3D12CommandQueue& queue,
			std::uint64_t* fenceValue);

		Status waitForFenceValue(std::uint64_t fenceValue);
		std::uint64_t completedFenceValue(void) const;

		ID3D12Fence* fence(void) const;

		/// The most recent value handed out by signal, or zero before the first signal.
		std::uint64_t lastSignaledValue(void) const;

	private:
		ComPtr<ID3D12Fence> m_Fence;
		HANDLE m_FenceEvent = nullptr;
		std::uint64_t m_NextFenceValue = 1;
	};
} // namespace spall::d3d12
