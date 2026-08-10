// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/Query/IQueryPool.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <string>
#include <vector>

namespace spall::d3d12
{
	class CommandList;
	class Device;
	class GraphicsQueue;

	class QueryPool final : public SharedObject<IQueryPool>
	{
	public:
		QueryPool(
			Device& device,
			const QueryPoolInfo& info,
			ComPtr<ID3D12QueryHeap> queryHeap,
			ComPtr<ID3D12Resource> resultBuffer);

		~QueryPool(void) override;

		RenderBackendType backendType(void) const override;
		QueryPoolInfo info(void) const override;

	private:
		Resource<Device> m_Device;

		std::string m_DebugName;
		QueryPoolInfo m_Info = {};

		ComPtr<ID3D12QueryHeap> m_QueryHeap;
		ComPtr<ID3D12Resource> m_ResultBuffer;

		/// Queue fence value that retires the submission which wrote each query, or zero when never written.
		std::vector<std::uint64_t> m_QueryFenceValues;

	private:
		friend class CommandList;
		friend class ComputeQueue;
		friend class Device;
		friend class GraphicsQueue;
	};
} // namespace spall::d3d12
