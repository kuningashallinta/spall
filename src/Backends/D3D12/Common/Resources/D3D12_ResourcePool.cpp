// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Common/Resources/D3D12_ResourcePool.h>

#include <src/Backends/D3D12/Common/Mappings/D3D12_HeapMappings.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Common/DXGI/DXGIError.h>

#include <mutex>
#include <utility>

namespace spall::d3d12
{
	ResourcePool::Entry* ResourcePool::findAvailableBuffer(
		std::uint64_t size,
		D3D12_HEAP_TYPE heapType,
		D3D12_RESOURCE_FLAGS flags)
	{
		for (Entry& entry : m_Entries)
		{
			if (entry.Available and (entry.HeapType == heapType) and
				(entry.Description.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) and
				(entry.Description.Flags == flags) and
				(entry.Description.Width >= size))
			{
				return &entry;
			}
		}

		return nullptr;
	}

	ResourcePool::Entry* ResourcePool::findAvailableTexture(
		const D3D12_RESOURCE_DESC& description)
	{
		for (Entry& entry : m_Entries)
		{
			if (entry.Available and
				(entry.Description.Dimension == description.Dimension) and
				(entry.Description.Width == description.Width) and
				(entry.Description.Height == description.Height) and
				(entry.Description.DepthOrArraySize == description.DepthOrArraySize) and
				(entry.Description.MipLevels == description.MipLevels) and
				(entry.Description.Format == description.Format) and
				(entry.Description.Flags == description.Flags))
			{
				return &entry;
			}
		}

		return nullptr;
	}

	Status ResourcePool::acquireBuffer(
		Device& device,
		std::uint64_t size,
		D3D12_HEAP_TYPE heapType,
		D3D12_RESOURCE_FLAGS flags,
		ComPtr<ID3D12Resource>* resource)
	{
		const std::lock_guard<std::mutex> guard(m_Mutex);

		Entry* available = findAvailableBuffer(size, heapType, flags);

		if (available != nullptr)
		{
			available->Available = false;
			*resource = available->Resource;

			return {};
		}

		D3D12_RESOURCE_DESC description = {};
		description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		description.Width = size;
		description.Height = 1;
		description.DepthOrArraySize = 1;
		description.MipLevels = 1;
		description.Format = DXGI_FORMAT_UNKNOWN;
		description.SampleDesc.Count = 1;
		description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		description.Flags = flags;

		const D3D12_RESOURCE_STATES initialState = (heapType == D3D12_HEAP_TYPE_UPLOAD)
			? D3D12_RESOURCE_STATE_GENERIC_READ
			: D3D12_RESOURCE_STATE_COMMON;

		const D3D12_HEAP_PROPERTIES properties = heapProperties(heapType);

		ComPtr<ID3D12Resource> created;
		const HRESULT hr = device.m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&description,
			initialState,
			nullptr,
			IID_PPV_ARGS(&created));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		Entry entry = {};
		entry.Resource = created;
		entry.Description = description;
		entry.HeapType = heapType;
		entry.Available = false;

		m_Entries.push_back(std::move(entry));
		*resource = std::move(created);

		return {};
	}

	Status ResourcePool::acquireTexture(
		Device& device,
		const D3D12_RESOURCE_DESC& description,
		D3D12_RESOURCE_STATES initialState,
		ComPtr<ID3D12Resource>* resource)
	{
		const std::lock_guard<std::mutex> guard(m_Mutex);

		Entry* available = findAvailableTexture(description);

		if (available != nullptr)
		{
			available->Available = false;
			*resource = available->Resource;

			return {};
		}

		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_DEFAULT);

		ComPtr<ID3D12Resource> created;
		const HRESULT hr = device.m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&description,
			initialState,
			nullptr,
			IID_PPV_ARGS(&created));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		Entry entry = {};
		entry.Resource = created;
		entry.Description = description;
		entry.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		entry.Available = false;

		m_Entries.push_back(std::move(entry));
		*resource = std::move(created);

		return {};
	}

	void ResourcePool::release(
		ComPtr<ID3D12Resource> resource)
	{
		const std::lock_guard<std::mutex> guard(m_Mutex);

		for (Entry& entry : m_Entries)
		{
			if (entry.Resource.Get() == resource.Get())
			{
				entry.Available = true;

				return;
			}
		}
	}
} // namespace spall::d3d12
