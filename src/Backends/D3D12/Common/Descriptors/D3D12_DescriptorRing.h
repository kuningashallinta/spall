// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>

namespace spall::d3d12
{
	/// Bump-allocates shader-visible descriptors for one recording.
	///
	/// A command list resets its rings only once its previous submission has
	/// retired, so allocations stay valid for as long as the GPU reads them.
	class DescriptorRing
	{
	public:
		Status initialize(
			ID3D12Device& device,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			std::uint32_t capacity);

		void reset(void);

		Status allocate(
			std::uint32_t count,
			D3D12_CPU_DESCRIPTOR_HANDLE* cpuStart,
			D3D12_GPU_DESCRIPTOR_HANDLE* gpuStart);

		ID3D12DescriptorHeap* heap(void) const;

	private:
		ComPtr<ID3D12DescriptorHeap> m_Heap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_CpuStart = {};
		D3D12_GPU_DESCRIPTOR_HANDLE m_GpuStart = {};

		std::uint32_t m_Capacity = 0;
		std::uint32_t m_Used = 0;
		std::uint32_t m_DescriptorSize = 0;
	};
} // namespace spall::d3d12
