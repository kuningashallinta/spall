// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorHeap.h>

#include <spall/Common/Assert.h>
#include <src/Common/DXGI/DXGIError.h>

#include <cstddef>
#include <mutex>

namespace spall::d3d12
{
	Status DescriptorHeap::initialize(
		ID3D12Device& device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		std::uint32_t capacity)
	{
		SPALL_ASSERT(capacity != 0);

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = type;
		heapDesc.NumDescriptors = capacity;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		const HRESULT hr = device.CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		m_Start = m_Heap->GetCPUDescriptorHandleForHeapStart();
		m_DescriptorSize = device.GetDescriptorHandleIncrementSize(type);
		m_Capacity = capacity;
		m_Used = 0;
		m_FreeIndices.clear();

		return {};
	}

	Status DescriptorHeap::allocate(
		std::uint32_t* index)
	{
		if (index == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		const std::lock_guard<std::mutex> guard(m_Mutex);

		if (not m_FreeIndices.empty())
		{
			*index = m_FreeIndices.back();
			m_FreeIndices.pop_back();

			return {};
		}

		if (m_Used == m_Capacity)
		{
			return ERR_OUT_OF_MEMORY;
		}

		*index = m_Used;
		++m_Used;

		return {};
	}

	void DescriptorHeap::release(
		std::uint32_t index)
	{
		if (index == InvalidDescriptorIndex)
		{
			return;
		}

		const std::lock_guard<std::mutex> guard(m_Mutex);

		SPALL_VERIFY(index < m_Used);
		m_FreeIndices.push_back(index);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpuHandle(
		std::uint32_t index) const
	{
		SPALL_ASSERT(index < m_Used);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_Start;
		handle.ptr += static_cast<SIZE_T>(index) * m_DescriptorSize;
		return handle;
	}
} // namespace spall::d3d12
