// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <mutex>
#include <vector>

namespace spall::d3d12
{
	class Device;

	/// Recycles the transient resources copies and mipmap generation need. Acquisition is thread-safe.
	///
	/// A pooled resource is handed back once the recording that borrowed it has
	/// retired, so an entry is only ever reused after its GPU work completed.
	class ResourcePool
	{
	public:
		Status acquireBuffer(
			Device& device,
			std::uint64_t size,
			D3D12_HEAP_TYPE heapType,
			D3D12_RESOURCE_FLAGS flags,
			ComPtr<ID3D12Resource>* resource);

		Status acquireTexture(
			Device& device,
			const D3D12_RESOURCE_DESC& description,
			D3D12_RESOURCE_STATES initialState,
			ComPtr<ID3D12Resource>* resource);

		void release(ComPtr<ID3D12Resource> resource);

	private:
		struct Entry
		{
			ComPtr<ID3D12Resource> Resource;
			D3D12_RESOURCE_DESC Description = {};
			D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
			bool Available = false;
		};

		Entry* findAvailableBuffer(
			std::uint64_t size,
			D3D12_HEAP_TYPE heapType,
			D3D12_RESOURCE_FLAGS flags);

		Entry* findAvailableTexture(const D3D12_RESOURCE_DESC& description);

		std::vector<Entry> m_Entries;
		std::mutex m_Mutex;
	};
} // namespace spall::d3d12
