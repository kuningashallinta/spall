// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Queue/D3D12_CrossQueueWait.h>

#include <src/Backends/D3D12/Queue/D3D12_ComputeQueue.h>
#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>
#include <src/Common/DXGI/DXGIError.h>

namespace spall::d3d12
{
	Status insertCrossQueueWait(
		ID3D12CommandQueue& waitingQueue,
		IQueue& other)
	{
		FenceTimeline* otherTimeline = nullptr;

		if (GraphicsQueue* graphicsQueue = dynamic_cast<GraphicsQueue*>(&other))
		{
			otherTimeline = &graphicsQueue->fenceTimeline();
		}
		else if (ComputeQueue* computeQueue = dynamic_cast<ComputeQueue*>(&other))
		{
			otherTimeline = &computeQueue->fenceTimeline();
		}

		if (otherTimeline == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		ID3D12Fence* otherFence = otherTimeline->fence();

		if (otherFence == nullptr)
		{
			return {};
		}

		const HRESULT hr = waitingQueue.Wait(otherFence, otherTimeline->lastSignaledValue());

		if (FAILED(hr))
		{
			return mapHResult(hr);
		}

		return {};
	}
} // namespace spall::d3d12
