// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/MemoryAccess.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall::d3d12
{
	inline D3D12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE heapType);

	inline D3D12_HEAP_TYPE bufferHeapType(MemoryAccess access);

	/// Reports whether a heap pins its resources to one state for their whole lifetime.
	inline bool isFixedStateHeap(
		D3D12_HEAP_TYPE heapType)
	{
		return (heapType == D3D12_HEAP_TYPE_UPLOAD) or (heapType == D3D12_HEAP_TYPE_READBACK);
	}

	inline D3D12_RESOURCE_STATES fixedHeapState(
		D3D12_HEAP_TYPE heapType)
	{
		return (heapType == D3D12_HEAP_TYPE_UPLOAD) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
	}
} // namespace spall::d3d12

#include <src/Backends/D3D12/Common/Mappings/D3D12_HeapMappings.inl>
