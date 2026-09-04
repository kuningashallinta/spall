// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Queue/D3D12_ComputeQueue.h>

#include <src/Backends/D3D12/CommandList/D3D12_CommandList.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Queue/D3D12_CrossQueueWait.h>
#include <src/Backends/D3D12/Resources/Query/D3D12_QueryPool.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>

namespace spall::d3d12
{
	ComputeQueue::ComputeQueue(
		Device& device,
		ID3D12CommandQueue& commandQueue)
		: m_Device(&device), m_CommandQueue(&commandQueue)
	{
	}

	Status ComputeQueue::initialize()
	{
		return m_FenceTimeline.initialize(*m_Device->m_Device.Get());
	}

	RenderBackendType ComputeQueue::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	FenceTimeline& ComputeQueue::fenceTimeline()
	{
		return m_FenceTimeline;
	}

	Status ComputeQueue::waitForQueue(
		IQueue& other)
	{
		return insertCrossQueueWait(*m_CommandQueue, other);
	}

	Status ComputeQueue::submit(
		ICommandList& commandList)
	{
		CommandList* backendCommandList = backendCast<CommandList>(commandList);

		if (backendCommandList == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendCommandList->m_Device.get() != m_Device)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendCommandList->m_ExecutionState != CommandList::ExecutionState::Executable)
		{
			return ERR_INVALID_STATE;
		}

		if (backendCommandList->m_CommandListType != D3D12_COMMAND_LIST_TYPE_COMPUTE)
		{
			return ERR_INVALID_STATE;
		}

		if (backendCommandList->m_ReferencedPresentTexture)
		{
			return ERR_INVALID_STATE;
		}

		ID3D12CommandList* const submittedCommandLists[] = {backendCommandList->m_CommandList.Get()};
		m_CommandQueue->ExecuteCommandLists(1, submittedCommandLists);

		std::uint64_t fenceValue = 0;
		SPALL_TRY(m_FenceTimeline.signal(*m_CommandQueue, &fenceValue));

		backendCommandList->m_StateTracker.commandListSubmitted();

		for (const auto& timestampWrite : backendCommandList->m_TimestampWrites)
		{
			timestampWrite.first->m_QueryFenceValues[timestampWrite.second] = fenceValue;
		}

		backendCommandList->m_SubmissionFence = &m_FenceTimeline;
		backendCommandList->m_SubmissionFenceValue = fenceValue;
		backendCommandList->m_ExecutionState = CommandList::ExecutionState::Pending;

		return {};
	}

	Status ComputeQueue::waitIdle()
	{
		if (m_FenceTimeline.fence() == nullptr)
		{
			return {};
		}

		std::uint64_t fenceValue = 0;
		SPALL_TRY(m_FenceTimeline.signal(*m_CommandQueue, &fenceValue));

		return m_FenceTimeline.waitForFenceValue(fenceValue);
	}
} // namespace spall::d3d12
