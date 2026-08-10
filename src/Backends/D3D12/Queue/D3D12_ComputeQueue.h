// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Queue/IQueue.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Backends/D3D12/Queue/D3D12_FenceTimeline.h>

namespace spall::d3d12
{
	class Device;

	class ComputeQueue final : public IQueue
	{
	public:
		ComputeQueue(
			Device& device,
			ID3D12CommandQueue& commandQueue);

		Status initialize(void);

		RenderBackendType backendType(void) const override;

		Status submit(ICommandList& commandList) override;
		Status waitForQueue(IQueue& other) override;
		Status waitIdle(void) override;

		FenceTimeline& fenceTimeline(void);

	private:
		Device* m_Device = nullptr;
		ID3D12CommandQueue* m_CommandQueue = nullptr;
		FenceTimeline m_FenceTimeline;

	private:
		friend class CommandList;
		friend class Device;
	};
} // namespace spall::d3d12
