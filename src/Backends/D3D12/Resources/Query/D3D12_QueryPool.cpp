// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Resources/Query/D3D12_QueryPool.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	QueryPool::QueryPool(
		Device& device,
		const QueryPoolInfo& info,
		ComPtr<ID3D12QueryHeap> queryHeap,
		ComPtr<ID3D12Resource> resultBuffer)
		: m_Device(&device), m_Info(info), m_QueryHeap(std::move(queryHeap)), m_ResultBuffer(std::move(resultBuffer))
	{
		m_QueryFenceValues.assign(info.TimestampCount, 0);

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

	QueryPool::~QueryPool() = default;

	RenderBackendType QueryPool::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	QueryPoolInfo QueryPool::info() const
	{
		return m_Info;
	}
} // namespace spall::d3d12
