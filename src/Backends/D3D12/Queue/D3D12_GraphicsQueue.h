#pragma once

#include <spall/Queue/IGraphicsQueue.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Backends/D3D12/Queue/D3D12_FenceTimeline.h>

#include <cstdint>
#include <vector>

namespace spall::d3d12
{
	class CommandList;
	class Device;
	class Frame;
	class SwapChain;

	class GraphicsQueue final : public IGraphicsQueue
	{
	public:
		GraphicsQueue(
			Device& device,
			ID3D12CommandQueue& commandQueue);

		Status initialize(void);

		RenderBackendType backendType(void) const override;

		Status acquireFrame(
			ISwapChain& swapChain,
			Resource<IFrame>* frame) override;

		Status submit(ICommandList& commandList) override;
		Status present(IFrame& frame) override;
		Status waitForQueue(IQueue& other) override;
		Status waitIdle(void) override;

		/// Signals the queue fence and reports the value that marks the submitted work.
		Status signal(std::uint64_t* fenceValue);

		Status waitForFenceValue(std::uint64_t fenceValue);
		std::uint64_t completedFenceValue(void) const;
		FenceTimeline& fenceTimeline(void);

	private:
		void forgetCommandList(CommandList* commandList);

		Device* m_Device = nullptr;
		ID3D12CommandQueue* m_CommandQueue = nullptr;

		FenceTimeline m_FenceTimeline;

		SwapChain* m_ActiveSwapChain = nullptr;
		Frame* m_ActiveFrame = nullptr;
		bool m_ActiveFrameSubmitted = false;
		std::vector<CommandList*> m_PresentCommandLists;

	private:
		friend class CommandList;
		friend class Frame;
		friend class SwapChain;
	};
} // namespace spall::d3d12
