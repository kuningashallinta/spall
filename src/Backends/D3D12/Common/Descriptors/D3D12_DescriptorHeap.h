// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <mutex>
#include <vector>

namespace spall::d3d12
{
	/// Allocates fixed-capacity descriptors from one non shader-visible heap. Allocation is thread-safe.
	class DescriptorHeap
	{
	public:
		Status initialize(
			ID3D12Device& device,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			std::uint32_t capacity);

		Status allocate(std::uint32_t* index);
		void release(std::uint32_t index);

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(std::uint32_t index) const;

	private:
		ComPtr<ID3D12DescriptorHeap> m_Heap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_Start = {};
		std::vector<std::uint32_t> m_FreeIndices;

		std::uint32_t m_Capacity = 0;
		std::uint32_t m_Used = 0;
		std::uint32_t m_DescriptorSize = 0;

		mutable std::mutex m_Mutex;
	};
} // namespace spall::d3d12
