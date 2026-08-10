// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Resources/Buffer/D3D12_Buffer.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	Buffer::Buffer(
		Device& device,
		const BufferInfo& info,
		ComPtr<ID3D12Resource> resource,
		D3D12_HEAP_TYPE heapType)
		: m_Device(&device), m_Info(info), m_Resource(std::move(resource)), m_HeapType(heapType), m_State(info.InitialState)
	{
		if (info.DebugName != nullptr)
		{
			m_DebugName = info.DebugName;
			m_Info.DebugName = m_DebugName.c_str();
		}
		else
		{
			m_Info.DebugName = nullptr;
		}
	}

	Buffer::~Buffer() = default;

	RenderBackendType Buffer::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	BufferInfo Buffer::info() const
	{
		return m_Info;
	}
} // namespace spall::d3d12
