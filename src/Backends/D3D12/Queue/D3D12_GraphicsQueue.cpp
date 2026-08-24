// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>

#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/CommandList/D3D12_CommandList.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Frame/D3D12_Frame.h>
#include <src/Backends/D3D12/Queue/D3D12_CrossQueueWait.h>
#include <src/Backends/D3D12/Resources/Query/D3D12_QueryPool.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Backends/D3D12/SwapChain/D3D12_SwapChain.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>
#include <limits>
#include <memory>

namespace spall::d3d12
{
	GraphicsQueue::GraphicsQueue(
		Device& device,
		ID3D12CommandQueue& commandQueue)
		: m_Device(&device), m_CommandQueue(&commandQueue)
	{
	}

	Status GraphicsQueue::initialize()
	{
		return m_FenceTimeline.initialize(*m_Device->m_Device.Get());
	}

	RenderBackendType GraphicsQueue::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	std::uint64_t GraphicsQueue::completedFenceValue() const
	{
		return m_FenceTimeline.completedFenceValue();
	}

	FenceTimeline& GraphicsQueue::fenceTimeline()
	{
		return m_FenceTimeline;
	}

	Status GraphicsQueue::waitForQueue(
		IQueue& other)
	{
		return insertCrossQueueWait(*m_CommandQueue, other);
	}

	Status GraphicsQueue::signal(
		std::uint64_t* fenceValue)
	{
		return m_FenceTimeline.signal(*m_CommandQueue, fenceValue);
	}

	Status GraphicsQueue::waitForFenceValue(
		std::uint64_t fenceValue)
	{
		return m_FenceTimeline.waitForFenceValue(fenceValue);
	}

	Status GraphicsQueue::acquireFrame(
		ISwapChain& swapChain,
		Resource<IFrame>* frame)
	{
		if (frame == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (m_ActiveFrame != nullptr)
		{
			return ERR_INVALID_STATE;
		}

		SwapChain* backendSwapChain = backendCast<SwapChain>(swapChain);

		if (backendSwapChain == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendSwapChain->m_Device.get() != m_Device)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((backendSwapChain->m_Width == 0) or (backendSwapChain->m_Height == 0))
		{
			return ERR_INVALID_SIZE;
		}

		if (backendSwapChain->m_BackBuffers.empty())
		{
			SPALL_TRY(backendSwapChain->recreateBackBuffers());
		}

		const std::uint32_t frameIndex = backendSwapChain->currentBackBufferIndex();

		if (frameIndex >= backendSwapChain->m_BackBuffers.size())
		{
			return ERR_SWAP_CHAIN_ACQUIRE_FAILED;
		}

		SPALL_TRY(waitForFenceValue(backendSwapChain->m_BackBuffers[frameIndex].FenceValue));

		std::unique_ptr<Frame> createdFrame = std::make_unique<Frame>(
			*backendSwapChain,
			frameIndex,
			*backendSwapChain->m_BackBuffers[frameIndex].Texture,
			*backendSwapChain->m_BackBuffers[frameIndex].View);

		m_ActiveFrame = createdFrame.get();
		m_ActiveSwapChain = backendSwapChain;
		m_ActiveFrameSubmitted = false;

		*frame = Resource<IFrame>(createdFrame.release());

		return {};
	}

	Status GraphicsQueue::submit(
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

		if (backendCommandList->m_CommandListType != D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			return ERR_INVALID_STATE;
		}

		if (backendCommandList->m_ReferencedPresentTexture)
		{
			if ((m_ActiveFrame == nullptr) or (m_ActiveSwapChain == nullptr))
			{
				return ERR_INVALID_STATE;
			}

			if (m_ActiveFrameSubmitted)
			{
				return ERR_INVALID_STATE;
			}

			if ((backendCommandList->m_ReferencedSwapChain != m_ActiveSwapChain) or
				(backendCommandList->m_ReferencedPresentTexture.get() != m_ActiveFrame->m_PresentTexture))
			{
				return ERR_INVALID_RESOURCE_STATE;
			}
		}

		ID3D12CommandList* const submittedCommandLists[] = {backendCommandList->m_CommandList.Get()};
		m_CommandQueue->ExecuteCommandLists(1, submittedCommandLists);

		std::uint64_t fenceValue = 0;
		SPALL_TRY(signal(&fenceValue));

		backendCommandList->m_StateTracker.commandListSubmitted();

		for (const auto& timestampWrite : backendCommandList->m_TimestampWrites)
		{
			timestampWrite.first->m_QueryFenceValues[timestampWrite.second] = fenceValue;
		}

		backendCommandList->m_SubmissionFence = &m_FenceTimeline;
		backendCommandList->m_SubmissionFenceValue = fenceValue;
		backendCommandList->m_ExecutionState = CommandList::ExecutionState::Pending;

		if (backendCommandList->m_ReferencedPresentTexture)
		{
			m_ActiveSwapChain->m_BackBuffers[m_ActiveFrame->m_Index].FenceValue = fenceValue;
			m_ActiveFrameSubmitted = true;

			if (backendCommandList->m_ReclaimQueue != this)
			{
				backendCommandList->m_ReclaimQueue = this;
				m_PresentCommandLists.push_back(backendCommandList);
			}
		}

		return {};
	}

	Status GraphicsQueue::present(
		IFrame& frame)
	{
		Frame* backendFrame = backendCast<Frame>(frame);

		if (backendFrame == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((m_ActiveFrame == nullptr) or (m_ActiveFrame != backendFrame) or (m_ActiveSwapChain == nullptr))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (not m_ActiveFrameSubmitted)
		{
			return ERR_INVALID_STATE;
		}

		const UINT syncInterval = (m_ActiveSwapChain->m_PresentMode == PresentMode::VSync) ? 1u : 0u;
		const UINT presentFlags = (m_ActiveSwapChain->m_PresentMode == PresentMode::Immediate) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		Status result = {};
		bool shouldPresent = true;

		if (m_ActiveSwapChain->m_Occluded)
		{
			const HRESULT testHr = m_ActiveSwapChain->m_SwapChain->Present(0, DXGI_PRESENT_TEST);

			if (testHr == DXGI_STATUS_OCCLUDED)
			{
				shouldPresent = false;
			}
			else if (FAILED(testHr))
			{
				shouldPresent = false;
				result = mapStatus(testHr);
			}
			else
			{
				m_ActiveSwapChain->m_Occluded = false;
			}
		}

		if (shouldPresent)
		{
			const HRESULT hr = m_ActiveSwapChain->m_SwapChain->Present(syncInterval, presentFlags);

			if (hr == DXGI_STATUS_OCCLUDED)
			{
				m_ActiveSwapChain->m_Occluded = true;
			}
			else if (FAILED(hr))
			{
				result = mapStatus(hr);
			}
		}

		m_ActiveFrame = nullptr;
		m_ActiveSwapChain = nullptr;
		m_ActiveFrameSubmitted = false;

		return result;
	}

	Status GraphicsQueue::waitIdle()
	{
		if (m_FenceTimeline.fence() == nullptr)
		{
			return {};
		}

		std::uint64_t fenceValue = 0;
		SPALL_TRY(signal(&fenceValue));
		SPALL_TRY(waitForFenceValue(fenceValue));

		for (CommandList* commandList : m_PresentCommandLists)
		{
			commandList->m_ReclaimQueue = nullptr;
			commandList->reclaimAfterCompletion();
		}

		m_PresentCommandLists.clear();

		return {};
	}

	void GraphicsQueue::forgetCommandList(
		CommandList* commandList)
	{
		for (auto iterator = m_PresentCommandLists.begin(); iterator != m_PresentCommandLists.end(); ++iterator)
		{
			if (*iterator == commandList)
			{
				m_PresentCommandLists.erase(iterator);
				return;
			}
		}
	}
} // namespace spall::d3d12
