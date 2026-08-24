// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/CommandList/D3D12_CommandList.h>

#include <spall/Common/Alignment.h>
#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Common/Resources/D3D12_CopyLayout.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_HeapMappings.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_RayTracingMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_ResourceStateMappings.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Framebuffer/D3D12_Framebuffer.h>
#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSet.h>
#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSetLayout.h>
#include <src/Backends/D3D12/Pipeline/ComputePipeline/D3D12_ComputePipeline.h>
#include <src/Backends/D3D12/Pipeline/GraphicsPipeline/D3D12_GraphicsPipeline.h>
#include <src/Backends/D3D12/Pipeline/RayTracingPipeline/D3D12_RayTracingPipeline.h>
#include <src/Backends/D3D12/Queue/D3D12_FenceTimeline.h>
#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>
#include <src/Backends/D3D12/Resources/AccelerationStructure/D3D12_AccelerationStructure.h>
#include <src/Backends/D3D12/Resources/Buffer/D3D12_Buffer.h>
#include <src/Backends/D3D12/Resources/Query/D3D12_QueryPool.h>
#include <src/Backends/D3D12/Resources/Sampler/D3D12_Sampler.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>
#include <src/Backends/D3D12/SwapChain/D3D12_SwapChain.h>
#include <src/Common/DXGI/DXGICopyLayout.h>
#include <src/Common/DXGI/DXGIDebugLabel.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace spall::d3d12
{
	CommandList::CommandList(
		Device& device,
		QueueType type)
		: m_Device(&device),
		  m_CommandListType(type == QueueType::Compute ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
	}

	Status CommandList::initialize()
	{
		HRESULT hr = m_Device->m_Device->CreateCommandAllocator(
			m_CommandListType,
			IID_PPV_ARGS(&m_CommandAllocator));

		if (FAILED(hr))
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapStatus(hr);
		}

		hr = m_Device->m_Device->CreateCommandList(
			0,
			m_CommandListType,
			m_CommandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&m_CommandList));

		if (FAILED(hr))
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapStatus(hr);
		}

		hr = m_CommandList->Close();

		if (FAILED(hr))
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapStatus(hr);
		}

		m_StateTracker.setCommandList(m_CommandList.Get());

		m_CommandList.As(&m_RayTracingCommandList);

		SPALL_TRY(m_Device->m_DescriptorRingPool.acquire(*m_Device->m_Device.Get(), &m_Rings));

		return {};
	}

	CommandList::~CommandList()
	{
		if (m_ReclaimQueue != nullptr)
		{
			m_ReclaimQueue->forgetCommandList(this);
			m_ReclaimQueue = nullptr;
		}

		if (m_ExecutionState == ExecutionState::Pending)
		{
			if (m_SubmissionFence != nullptr)
			{
				m_SubmissionFence->waitForFenceValue(m_SubmissionFenceValue);
			}
		}

		m_Device->m_DescriptorRingPool.release(std::move(m_Rings));
		releaseScratchResources();
	}

	void CommandList::releaseScratchResources()
	{
		for (ComPtr<ID3D12Resource>& scratchResource : m_RetainedScratchBuffers)
		{
			m_Device->m_ResourcePool.release(std::move(scratchResource));
		}

		m_RetainedScratchBuffers.clear();
		m_RetiredAccelerationStructures.clear();
	}

	RenderBackendType CommandList::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	Status CommandList::fail(
		Status error)
	{
		if ((m_ExecutionState == ExecutionState::Recording) and m_CommandList)
		{
			m_CommandList->Close();
		}

		if (m_ExecutionState != ExecutionState::Pending)
		{
			m_ExecutionState = ExecutionState::Invalid;
		}

		return error;
	}

	void CommandList::retainResource(
		IResource& resource)
	{
		m_RetainedResources.push_back(Resource<IResource>(&resource));
	}

	Status CommandList::requireTextureState(
		Texture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		if (not m_AutomaticBarriers)
		{
			return {};
		}

		return m_StateTracker.requireTextureState(texture, state, subresources);
	}

	Status CommandList::requireBufferState(
		Buffer& buffer,
		ResourceStateFlags state)
	{
		if (not m_AutomaticBarriers)
		{
			return {};
		}

		return m_StateTracker.requireBufferState(buffer, state);
	}

	Status CommandList::requireTextureViewState(
		TextureView& textureView,
		ResourceStateFlags state)
	{
		return requireTextureState(
			*textureView.m_Texture,
			state,
			TextureSubresourceRange {
				textureView.m_BaseMipLevel,
				textureView.m_MipLevels,
				textureView.m_BaseArrayLayer,
				textureView.m_ArrayLayers});
	}

	Status CommandList::createScratchBuffer(
		std::uint64_t size,
		D3D12_RESOURCE_FLAGS flags,
		ID3D12Resource** resource)
	{
		SPALL_ASSERT(resource != nullptr);

		ComPtr<ID3D12Resource> scratchBuffer;
		SPALL_TRY(m_Device->m_ResourcePool.acquireBuffer(
			*m_Device,
			size,
			D3D12_HEAP_TYPE_DEFAULT,
			flags,
			&scratchBuffer));

		*resource = scratchBuffer.Get();
		m_RetainedScratchBuffers.push_back(std::move(scratchBuffer));

		return {};
	}

	void CommandList::transitionScratchBuffer(
		ID3D12Resource& resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = &resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		m_CommandList->ResourceBarrier(1, &barrier);
	}

	void CommandList::transitionScratchSubresource(
		ID3D12Resource& resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = &resource;
		barrier.Transition.Subresource = subresource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		m_CommandList->ResourceBarrier(1, &barrier);
	}

	Status CommandList::createMipmapScratchTexture(
		const TextureInfo& info,
		ComPtr<ID3D12Resource>* resource)
	{
		SPALL_ASSERT(resource != nullptr);

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = info.Width;
		resourceDesc.Height = info.Height;
		resourceDesc.DepthOrArraySize = static_cast<UINT16>(info.ArrayLayers);
		resourceDesc.MipLevels = static_cast<UINT16>(info.MipLevels);
		resourceDesc.Format = format(info.Format);
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		ComPtr<ID3D12Resource> scratch;
		SPALL_TRY(m_Device->m_ResourcePool.acquireTexture(
			*m_Device,
			resourceDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&scratch));

		*resource = scratch;
		m_RetainedScratchBuffers.push_back(std::move(scratch));

		return {};
	}

	Status CommandList::referencePresentTexture(
		Texture* texture)
	{
		if ((texture != nullptr) and (texture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((texture == nullptr) or (not texture->m_IsSwapChainTexture))
		{
			return {};
		}

		if (m_ReferencedPresentTexture and (m_ReferencedPresentTexture.get() != texture))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		retainResource(*texture);
		m_ReferencedPresentTexture.reset(texture);
		m_ReferencedSwapChain = texture->m_SwapChain;

		return {};
	}

	void CommandList::releaseResourceSetReferences()
	{
		for (Resource<ResourceSet>& boundSet : m_BoundResourceSets)
		{
			if (boundSet)
			{
				SPALL_VERIFY(boundSet->m_CommandListReferenceCount != 0);
				--boundSet->m_CommandListReferenceCount;
				boundSet.reset();
			}
		}
	}

	void CommandList::resetTransientState()
	{
		releaseResourceSetReferences();

		m_GraphicsPipeline = nullptr;
		m_ComputePipeline = nullptr;
		m_RayTracingPipeline = nullptr;
		m_PushConstantData.clear();
		m_IndexBufferSet = false;

		m_ReferencedPresentTexture.reset();
		m_ReferencedSwapChain = nullptr;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < MaxColorAttachments; ++attachmentIndex)
		{
			m_RenderPassColorTextures[attachmentIndex] = nullptr;
			m_RenderPassResolveTextures[attachmentIndex] = nullptr;
		}

		m_RenderPassColorTextureCount = 0;
		m_RenderPassDepthTexture = nullptr;
		m_DebugGroupDepth = 0;
		m_RenderPassActive = false;
		m_ViewportSet = false;
		m_ScissorSet = false;

		m_StateTracker.reset();
		m_TimestampWrites.clear();
		m_RetainedResources.clear();
		releaseScratchResources();
	}

	void CommandList::reclaimAfterCompletion()
	{
		if (m_ExecutionState != ExecutionState::Pending)
		{
			return;
		}

		m_ExecutionState = ExecutionState::Completed;
		resetTransientState();
	}

	Status CommandList::begin()
	{
		if ((not m_Device) or (not m_CommandList) or (not m_CommandAllocator))
		{
			return ERR_INVALID_STATE;
		}

		if (m_ExecutionState == ExecutionState::Pending)
		{
			if ((m_SubmissionFence != nullptr) and (m_SubmissionFence->completedFenceValue() < m_SubmissionFenceValue))
			{
				return ERR_INVALID_STATE;
			}

			m_ExecutionState = ExecutionState::Completed;
		}

		if (m_ExecutionState == ExecutionState::Invalid)
		{
			m_ExecutionState = ExecutionState::Initial;
			m_RenderPassActive = false;
		}

		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, false));

		resetTransientState();

		HRESULT hr = m_CommandAllocator->Reset();

		if (FAILED(hr))
		{
			return fail(mapStatus(hr));
		}

		hr = m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);

		if (FAILED(hr))
		{
			return fail(mapStatus(hr));
		}

		m_Rings.Views.reset();
		m_Rings.Samplers.reset();

		ID3D12DescriptorHeap* const descriptorHeaps[] = {m_Rings.Views.heap(), m_Rings.Samplers.heap()};
		m_CommandList->SetDescriptorHeaps(2, descriptorHeaps);

		m_ExecutionState = ExecutionState::Recording;

		return {};
	}

	Status CommandList::end()
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_DebugGroupDepth != 0)
		{
			return ERR_INVALID_STATE;
		}

		if (m_RenderPassActive)
		{
			SPALL_TRY(endRenderPass());
		}

		resolveTimestampWrites();

		if (m_ReferencedPresentTexture)
		{
			Status error = m_StateTracker.requireTextureState(*m_ReferencedPresentTexture, ResourceStateFlags::Present);

			if (error != SUCCESS)
			{
				return fail(error);
			}
		}

		Status error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		const HRESULT hr = m_CommandList->Close();

		if (FAILED(hr))
		{
			m_ExecutionState = ExecutionState::Invalid;

			return mapStatus(hr);
		}

		m_ExecutionState = ExecutionState::Executable;

		return {};
	}

	Status CommandList::pushDebugGroup(
		const char* label,
		Color color)
	{
		(void)color;

		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));
		SPALL_TRY(validateDebugLabel(label));

		std::wstring wideLabel;
		SPALL_TRY(wideDebugLabel(label, &wideLabel));

		m_CommandList->BeginEvent(UnicodeEventVersion, wideLabel.c_str(), eventPayloadSize(wideLabel));
		++m_DebugGroupDepth;

		return {};
	}

	Status CommandList::popDebugGroup()
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_DebugGroupDepth == 0)
		{
			return ERR_INVALID_STATE;
		}

		m_CommandList->EndEvent();
		--m_DebugGroupDepth;

		return {};
	}

	Status CommandList::insertDebugMarker(
		const char* label,
		Color color)
	{
		(void)color;

		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));
		SPALL_TRY(validateDebugLabel(label));

		std::wstring wideLabel;
		SPALL_TRY(wideDebugLabel(label, &wideLabel));

		m_CommandList->SetMarker(UnicodeEventVersion, wideLabel.c_str(), eventPayloadSize(wideLabel));

		return {};
	}

	Status CommandList::beginRenderPass(
		const RenderPassBeginInfo& beginInfo)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		SPALL_TRY(validatePassBeginInfo(beginInfo));

		Framebuffer* framebuffer = backendCast<Framebuffer>(beginInfo.Framebuffer);

		if (framebuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (framebuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		retainResource(*framebuffer);

		D3D12_CPU_DESCRIPTOR_HANDLE colorHandles[MaxColorAttachments] = {};

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			TextureView* colorView = framebuffer->m_ColorViews[attachmentIndex].get();

			Status error = referencePresentTexture(colorView->m_Texture.get());

			if (error != SUCCESS)
			{
				return fail(error);
			}

			error = requireTextureViewState(*colorView, ResourceStateFlags::RenderTarget);

			if (error != SUCCESS)
			{
				return fail(error);
			}

			colorHandles[attachmentIndex] = m_Device->m_RenderTargetViews.cpuHandle(colorView->m_RenderTargetViewIndex);
			m_RenderPassColorTextures[attachmentIndex] = colorView->m_Texture.get();

			TextureView* const resolveView = framebuffer->m_ResolveViews[attachmentIndex].get();
			m_RenderPassResolveTextures[attachmentIndex] = resolveView != nullptr ? resolveView->m_Texture.get() : nullptr;
		}

		TextureView* depthView = framebuffer->m_DepthView.get();
		D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = {};

		if (depthView != nullptr)
		{
			Status error = requireTextureViewState(*depthView, ResourceStateFlags::DepthWrite);

			if (error != SUCCESS)
			{
				return fail(error);
			}

			depthHandle = m_Device->m_DepthStencilViews.cpuHandle(depthView->m_DepthStencilViewIndex);
			m_RenderPassDepthTexture = depthView->m_Texture.get();
		}

		Status error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		m_CommandList->OMSetRenderTargets(
			static_cast<UINT>(framebuffer->m_ColorCount),
			(framebuffer->m_ColorCount != 0) ? colorHandles : nullptr,
			FALSE,
			(depthView != nullptr) ? &depthHandle : nullptr);

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < framebuffer->m_ColorCount; ++attachmentIndex)
		{
			const ColorAttachmentInfo& attachment = beginInfo.ColorAttachments[attachmentIndex];

			if (attachment.LoadAction != LoadAction::Clear)
			{
				continue;
			}

			const FLOAT clearColor[4] = {
				attachment.ClearColor.R,
				attachment.ClearColor.G,
				attachment.ClearColor.B,
				attachment.ClearColor.A};

			m_CommandList->ClearRenderTargetView(colorHandles[attachmentIndex], clearColor, 0, nullptr);
		}

		if (depthView != nullptr)
		{
			D3D12_CLEAR_FLAGS clearFlags = {};

			if (beginInfo.DepthAttachment.DepthLoadAction == LoadAction::Clear)
			{
				clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
			}

			if ((beginInfo.DepthAttachment.StencilLoadAction == LoadAction::Clear) and
				hasStencilAspect(depthView->m_Texture->m_Info.Format))
			{
				clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
			}

			if (clearFlags != 0)
			{
				m_CommandList->ClearDepthStencilView(
					depthHandle,
					clearFlags,
					beginInfo.DepthAttachment.ClearDepth,
					beginInfo.DepthAttachment.ClearStencil,
					0,
					nullptr);
			}
		}

		m_RenderPassColorTextureCount = framebuffer->m_ColorCount;
		m_RenderPassActive = true;

		return {};
	}

	Status CommandList::endRenderPass()
	{
		if (not m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		m_CommandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
		m_RenderPassActive = false;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < m_RenderPassColorTextureCount; ++attachmentIndex)
		{
			Texture* const resolveTexture = m_RenderPassResolveTextures[attachmentIndex];

			if (resolveTexture == nullptr)
			{
				continue;
			}

			Status resolveError = m_StateTracker.requireTextureState(*resolveTexture, ResourceStateFlags::RenderTarget);

			if (resolveError != SUCCESS)
			{
				return fail(resolveError);
			}
		}

		Status error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < m_RenderPassColorTextureCount; ++attachmentIndex)
		{
			Texture* const sourceTexture = m_RenderPassColorTextures[attachmentIndex];
			Texture* const resolveTexture = m_RenderPassResolveTextures[attachmentIndex];

			if (sourceTexture == nullptr or resolveTexture == nullptr)
			{
				continue;
			}

			ID3D12Resource& sourceResource = *sourceTexture->m_Resource.Get();
			ID3D12Resource& resolveResource = *resolveTexture->m_Resource.Get();

			transitionScratchBuffer(sourceResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
			transitionScratchBuffer(resolveResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST);

			m_CommandList->ResolveSubresource(&resolveResource, 0, &sourceResource, 0, resolveResource.GetDesc().Format);

			transitionScratchBuffer(sourceResource, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			transitionScratchBuffer(resolveResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < m_RenderPassColorTextureCount; ++attachmentIndex)
		{
			m_RenderPassColorTextures[attachmentIndex] = nullptr;
			m_RenderPassResolveTextures[attachmentIndex] = nullptr;
		}

		m_RenderPassColorTextureCount = 0;
		m_RenderPassDepthTexture = nullptr;

		return {};
	}

	Status CommandList::setViewport(
		const Viewport& viewport)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));
		SPALL_TRY(validateViewport(viewport));

		D3D12_VIEWPORT nativeViewport = {};
		nativeViewport.TopLeftX = viewport.X;
		nativeViewport.TopLeftY = viewport.Y;
		nativeViewport.Width = viewport.Width;
		nativeViewport.Height = viewport.Height;
		nativeViewport.MinDepth = viewport.MinDepth;
		nativeViewport.MaxDepth = viewport.MaxDepth;

		m_CommandList->RSSetViewports(1, &nativeViewport);
		m_ViewportSet = true;

		return {};
	}

	Status CommandList::setScissor(
		const Scissor& scissor)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));
		SPALL_TRY(validateScissor(scissor));

		D3D12_RECT nativeScissor = {};
		nativeScissor.left = scissor.X;
		nativeScissor.top = scissor.Y;
		nativeScissor.right = scissor.X + static_cast<LONG>(scissor.Width);
		nativeScissor.bottom = scissor.Y + static_cast<LONG>(scissor.Height);

		m_CommandList->RSSetScissorRects(1, &nativeScissor);
		m_ScissorSet = true;

		return {};
	}

	Status CommandList::setEnableAutomaticBarriers(
		bool enable)
	{
		m_AutomaticBarriers = enable;

		return {};
	}

	Status CommandList::beginTrackingTextureState(
		ITexture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Texture* backendTexture = backendCast<Texture>(texture);

		if (backendTexture == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendTexture->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateTextureSubresourceRange(backendTexture->m_Info, subresources));
		SPALL_TRY(validateTextureResourceState(backendTexture->m_Info, state, backendTexture->m_IsSwapChainTexture));

		return m_StateTracker.beginTrackingTextureState(*backendTexture, state, subresources);
	}

	Status CommandList::setTextureState(
		ITexture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Texture* backendTexture = backendCast<Texture>(texture);

		if (backendTexture == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendTexture->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateTextureSubresourceRange(backendTexture->m_Info, subresources));

		return m_StateTracker.requireTextureState(*backendTexture, state, subresources);
	}

	Status CommandList::setPermanentTextureState(
		ITexture& texture,
		ResourceStateFlags state)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Texture* backendTexture = backendCast<Texture>(texture);

		if (backendTexture == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendTexture->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendTexture->m_IsSwapChainTexture)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		return m_StateTracker.setPermanentTextureState(*backendTexture, state);
	}

	Status CommandList::commitBarriers()
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Status error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		return {};
	}

	ResourceStateFlags CommandList::textureState(
		ITexture& texture,
		const TextureSubresourceRange& subresources) const
	{
		Texture* backendTexture = backendCast<Texture>(texture);

		if ((backendTexture == nullptr) or (backendTexture->m_Device.get() != m_Device.get()))
		{
			return ResourceStateFlags::Unknown;
		}

		return m_StateTracker.currentTextureState(*backendTexture, subresources);
	}

	Status CommandList::beginTrackingBufferState(
		IBuffer& buffer,
		ResourceStateFlags state)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateBufferResourceState(backendBuffer->m_Info, state));

		return m_StateTracker.beginTrackingBufferState(*backendBuffer, state);
	}

	Status CommandList::setBufferState(
		IBuffer& buffer,
		ResourceStateFlags state)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		return m_StateTracker.requireBufferState(*backendBuffer, state);
	}

	Status CommandList::setPermanentBufferState(
		IBuffer& buffer,
		ResourceStateFlags state)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		return m_StateTracker.setPermanentBufferState(*backendBuffer, state);
	}

	ResourceStateFlags CommandList::bufferState(
		IBuffer& buffer) const
	{
		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if ((backendBuffer == nullptr) or (backendBuffer->m_Device.get() != m_Device.get()))
		{
			return ResourceStateFlags::Unknown;
		}

		return m_StateTracker.currentBufferState(*backendBuffer);
	}

	Status CommandList::setStencilReference(
		std::uint8_t reference)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		m_CommandList->OMSetStencilRef(reference);

		return {};
	}

	Status CommandList::setVertexBuffer(
		std::uint32_t slot,
		IBuffer& buffer,
		std::uint32_t stride,
		std::uint32_t offset)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((backendBuffer->m_Info.Usage & BufferUsageFlags::Vertex) == BufferUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((slot >= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) or (stride == 0) or (offset >= backendBuffer->m_Info.Size))
		{
			return ERR_INVALID_RANGE;
		}

		Status error = requireBufferState(*backendBuffer, ResourceStateFlags::VertexBuffer);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(buffer);

		D3D12_VERTEX_BUFFER_VIEW view = {};
		view.BufferLocation = backendBuffer->m_Resource->GetGPUVirtualAddress() + offset;
		view.SizeInBytes = backendBuffer->m_Info.Size - offset;
		view.StrideInBytes = stride;

		m_CommandList->IASetVertexBuffers(slot, 1, &view);

		return {};
	}

	Status CommandList::setIndexBuffer(
		IBuffer& buffer,
		IndexFormat format,
		std::uint32_t offset)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((backendBuffer->m_Info.Usage & BufferUsageFlags::Index) == BufferUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (offset >= backendBuffer->m_Info.Size)
		{
			return ERR_INVALID_RANGE;
		}

		Status error = requireBufferState(*backendBuffer, ResourceStateFlags::IndexBuffer);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(buffer);

		D3D12_INDEX_BUFFER_VIEW view = {};
		view.BufferLocation = backendBuffer->m_Resource->GetGPUVirtualAddress() + offset;
		view.SizeInBytes = backendBuffer->m_Info.Size - offset;
		view.Format = indexFormat(format);

		m_CommandList->IASetIndexBuffer(&view);
		m_IndexBufferSet = true;

		return {};
	}

	Status CommandList::bindGraphicsPipeline(
		IPipeline& pipeline)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (pipeline.type() != PipelineType::Graphics)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		GraphicsPipeline* backendPipeline = backendCast<GraphicsPipeline>(pipeline);

		if ((backendPipeline == nullptr) or (backendPipeline->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		retainResource(pipeline);

		m_CommandList->SetGraphicsRootSignature(backendPipeline->m_RootSignature->m_RootSignature.Get());
		m_CommandList->SetPipelineState(backendPipeline->m_PipelineState.Get());
		m_CommandList->IASetPrimitiveTopology(backendPipeline->m_PrimitiveTopology);
		m_CommandList->OMSetStencilRef(backendPipeline->m_StencilReference);

		m_GraphicsPipeline = backendPipeline;
		m_RayTracingPipeline = nullptr;
		m_PushConstantData.assign(backendPipeline->m_RootSignature->m_PushConstantSize, std::byte {});

		return {};
	}

	Status CommandList::bindComputePipeline(
		IPipeline& pipeline)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (pipeline.type() != PipelineType::Compute)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		ComputePipeline* backendPipeline = backendCast<ComputePipeline>(pipeline);

		if ((backendPipeline == nullptr) or (backendPipeline->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		retainResource(pipeline);

		m_CommandList->SetComputeRootSignature(backendPipeline->m_RootSignature->m_RootSignature.Get());
		m_CommandList->SetPipelineState(backendPipeline->m_PipelineState.Get());

		m_ComputePipeline = backendPipeline;
		m_RayTracingPipeline = nullptr;
		m_PushConstantData.assign(backendPipeline->m_RootSignature->m_PushConstantSize, std::byte {});

		return {};
	}

	Status CommandList::bindResourceSet(
		std::uint32_t slot,
		IResourceSet& resourceSet)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (slot >= MaxResourceSets)
		{
			return ERR_INVALID_BINDING;
		}

		ResourceSet* backendResourceSet = backendCast<ResourceSet>(resourceSet);

		if ((backendResourceSet == nullptr) or (backendResourceSet->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (m_BoundResourceSets[slot])
		{
			SPALL_VERIFY(m_BoundResourceSets[slot]->m_CommandListReferenceCount != 0);
			--m_BoundResourceSets[slot]->m_CommandListReferenceCount;
		}

		m_BoundResourceSets[slot].reset(backendResourceSet);
		++backendResourceSet->m_CommandListReferenceCount;

		return {};
	}

	Status CommandList::setPushConstants(
		ShaderStageFlags stages,
		std::uint32_t offset,
		std::span<const std::byte> data)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		const RootSignature* rootSignature = nullptr;

		if (m_GraphicsPipeline != nullptr)
		{
			rootSignature = m_GraphicsPipeline->m_RootSignature.get();
		}
		else if (m_ComputePipeline != nullptr)
		{
			rootSignature = m_ComputePipeline->m_RootSignature.get();
		}
		else if (m_RayTracingPipeline != nullptr)
		{
			rootSignature = m_RayTracingPipeline->m_RootSignature.get();
		}

		if (rootSignature == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		if (rootSignature->m_PushConstantSize == 0)
		{
			return ERR_INVALID_STATE;
		}

		if (stages != rootSignature->m_PushConstantStages)
		{
			return ERR_INVALID_BINDING;
		}

		if (((offset % 4) != 0) or ((data.size() % 4) != 0) or
			(offset > rootSignature->m_PushConstantSize) or
			(data.size() > (rootSignature->m_PushConstantSize - offset)))
		{
			return ERR_INVALID_RANGE;
		}

		std::memcpy(m_PushConstantData.data() + offset, data.data(), data.size());

		return {};
	}

	Status CommandList::requireResourceSetStates(
		const RootSignature& rootSignature)
	{
		for (std::uint32_t slot = 0; slot < rootSignature.setCount(); ++slot)
		{
			ResourceSet* boundSet = m_BoundResourceSets[slot].get();

			if (boundSet == nullptr)
			{
				return ERR_INVALID_BINDING;
			}

			for (std::size_t bindingIndex = 0; bindingIndex < boundSet->m_Layout->m_Bindings.size(); ++bindingIndex)
			{
				const ResourceBindingInfo& bindingInfo = boundSet->m_Layout->m_Bindings[bindingIndex];
				const ResourceSet::BoundResource& bound = boundSet->m_BoundResources[bindingIndex];

				if (not bound.Written)
				{
					return ERR_INVALID_BINDING;
				}

				switch (bindingInfo.Type)
				{
					case ResourceBindingType::UniformBuffer:
					{
						SPALL_TRY(requireBufferState(*bound.Buffer, ResourceStateFlags::ConstantBuffer));
						break;
					}

					case ResourceBindingType::StorageBuffer:
					{
						SPALL_TRY(requireBufferState(*bound.Buffer, ResourceStateFlags::UnorderedAccess));
						break;
					}

					case ResourceBindingType::SampledTexture:
					{
						SPALL_TRY(requireTextureState(*bound.TextureView->m_Texture, ResourceStateFlags::ShaderResource));
						break;
					}

					case ResourceBindingType::AccelerationStructure:
					{
						if (not bound.AccelerationStructure->m_Built)
						{
							return ERR_INVALID_STATE;
						}

						break;
					}

					case ResourceBindingType::StorageTexture:
					default:
					{
						SPALL_TRY(requireTextureState(*bound.TextureView->m_Texture, ResourceStateFlags::UnorderedAccess));
						break;
					}
				}
			}
		}

		return {};
	}

	Status CommandList::applyResourceSets(
		const RootSignature& rootSignature,
		PipelineType type)
	{
		const bool graphics = (type == PipelineType::Graphics);

		for (std::uint32_t slot = 0; slot < rootSignature.setCount(); ++slot)
		{
			const RootSignature::SetTables& tables = rootSignature.setTables(slot);
			ResourceSet* boundSet = m_BoundResourceSets[slot].get();

			if (boundSet->m_Layout.get() != rootSignature.layout(slot))
			{
				return ERR_INVALID_BINDING;
			}

			if (tables.ViewDescriptorCount != 0)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE destinationStart = {};
				D3D12_GPU_DESCRIPTOR_HANDLE tableStart = {};
				SPALL_TRY(m_Rings.Views.allocate(tables.ViewDescriptorCount, &destinationStart, &tableStart));

				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> sourceHandles;
				std::vector<UINT> sourceCounts;
				sourceHandles.reserve(tables.ViewDescriptorCount);
				sourceCounts.assign(tables.ViewDescriptorCount, 1);

				for (const std::uint32_t descriptorIndex : boundSet->m_ViewDescriptorIndices)
				{
					sourceHandles.push_back(m_Device->m_ShaderResourceDescriptors.cpuHandle(descriptorIndex));
				}

				const UINT destinationCount = tables.ViewDescriptorCount;
				m_Device->m_Device->CopyDescriptors(
					1,
					&destinationStart,
					&destinationCount,
					tables.ViewDescriptorCount,
					sourceHandles.data(),
					sourceCounts.data(),
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				if (graphics)
				{
					m_CommandList->SetGraphicsRootDescriptorTable(tables.ViewParameterIndex, tableStart);
				}
				else
				{
					m_CommandList->SetComputeRootDescriptorTable(tables.ViewParameterIndex, tableStart);
				}
			}

			if (tables.SamplerDescriptorCount != 0)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE destinationStart = {};
				D3D12_GPU_DESCRIPTOR_HANDLE tableStart = {};
				SPALL_TRY(m_Rings.Samplers.allocate(tables.SamplerDescriptorCount, &destinationStart, &tableStart));

				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> sourceHandles;
				std::vector<UINT> sourceCounts;
				sourceHandles.reserve(tables.SamplerDescriptorCount);
				sourceCounts.assign(tables.SamplerDescriptorCount, 1);

				for (std::size_t bindingIndex = 0; bindingIndex < boundSet->m_Layout->m_Bindings.size(); ++bindingIndex)
				{
					if (boundSet->m_Layout->m_Bindings[bindingIndex].Type != ResourceBindingType::SampledTexture)
					{
						continue;
					}

					sourceHandles.push_back(
						m_Device->m_SamplerDescriptors.cpuHandle(
							boundSet->m_BoundResources[bindingIndex].Sampler->m_DescriptorIndex));
				}

				const UINT destinationCount = tables.SamplerDescriptorCount;
				m_Device->m_Device->CopyDescriptors(
					1,
					&destinationStart,
					&destinationCount,
					tables.SamplerDescriptorCount,
					sourceHandles.data(),
					sourceCounts.data(),
					D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

				if (graphics)
				{
					m_CommandList->SetGraphicsRootDescriptorTable(tables.SamplerParameterIndex, tableStart);
				}
				else
				{
					m_CommandList->SetComputeRootDescriptorTable(tables.SamplerParameterIndex, tableStart);
				}
			}
		}

		return {};
	}

	void CommandList::applyPushConstants(
		const RootSignature& rootSignature,
		PipelineType type)
	{
		const bool graphics = (type == PipelineType::Graphics);

		if (rootSignature.m_PushConstantParameterIndex == InvalidRootParameter)
		{
			return;
		}

		const UINT valueCount = rootSignature.m_PushConstantSize / 4;

		if (graphics)
		{
			m_CommandList->SetGraphicsRoot32BitConstants(
				rootSignature.m_PushConstantParameterIndex,
				valueCount,
				m_PushConstantData.data(),
				0);
		}
		else
		{
			m_CommandList->SetComputeRoot32BitConstants(
				rootSignature.m_PushConstantParameterIndex,
				valueCount,
				m_PushConstantData.data(),
				0);
		}
	}

	Status CommandList::prepareDraw(
		bool indexed)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (not m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (m_GraphicsPipeline == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		if ((not m_ViewportSet) or (not m_ScissorSet))
		{
			return ERR_INVALID_STATE;
		}

		if (indexed and (not m_IndexBufferSet))
		{
			return ERR_INVALID_STATE;
		}

		const RootSignature& rootSignature = *m_GraphicsPipeline->m_RootSignature;

		SPALL_TRY(requireResourceSetStates(rootSignature));
		SPALL_TRY(m_StateTracker.commitBarriers());
		SPALL_TRY(applyResourceSets(rootSignature, PipelineType::Graphics));

		applyPushConstants(rootSignature, PipelineType::Graphics);

		return {};
	}

	Status CommandList::prepareDispatch()
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (m_ComputePipeline == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		const RootSignature& rootSignature = *m_ComputePipeline->m_RootSignature;

		SPALL_TRY(requireResourceSetStates(rootSignature));
		SPALL_TRY(m_StateTracker.commitBarriers());
		SPALL_TRY(applyResourceSets(rootSignature, PipelineType::Compute));

		applyPushConstants(rootSignature, PipelineType::Compute);

		return {};
	}

	Status CommandList::draw(
		std::uint32_t vertexCount,
		std::uint32_t startVertex,
		std::uint32_t instanceCount,
		std::uint32_t startInstance)
	{
		Status error = prepareDraw(false);

		if (error != SUCCESS)
		{
			return error;
		}

		m_CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);

		return {};
	}

	Status CommandList::drawIndexed(
		std::uint32_t indexCount,
		std::uint32_t startIndex,
		std::int32_t vertexOffset,
		std::uint32_t instanceCount,
		std::uint32_t startInstance)
	{
		Status error = prepareDraw(true);

		if (error != SUCCESS)
		{
			return error;
		}

		m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, vertexOffset, startInstance);

		return {};
	}

	Status CommandList::dispatch(
		std::uint32_t groupCountX,
		std::uint32_t groupCountY,
		std::uint32_t groupCountZ)
	{
		Status error = prepareDispatch();

		if (error != SUCCESS)
		{
			return error;
		}

		m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);

		return {};
	}

	Status CommandList::bindRayTracingPipeline(
		IPipeline& pipeline)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if ((not m_Device->m_RayTracingDevice) or (not m_RayTracingCommandList))
		{
			return ERR_UNSUPPORTED;
		}

		if (m_CommandListType != D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			return ERR_UNSUPPORTED;
		}

		if (pipeline.type() != PipelineType::RayTracing)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		RayTracingPipeline* backendPipeline = backendCast<RayTracingPipeline>(pipeline);

		if ((backendPipeline == nullptr) or (backendPipeline->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		retainResource(pipeline);

		m_CommandList->SetComputeRootSignature(backendPipeline->m_RootSignature->m_RootSignature.Get());
		m_RayTracingCommandList->SetPipelineState1(backendPipeline->m_StateObject.Get());

		m_RayTracingPipeline = backendPipeline;
		m_GraphicsPipeline = nullptr;
		m_ComputePipeline = nullptr;
		m_PushConstantData.assign(backendPipeline->m_RootSignature->m_PushConstantSize, std::byte {});

		return {};
	}

	Status CommandList::prepareRayDispatch()
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (m_RayTracingPipeline == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		const RootSignature& rootSignature = *m_RayTracingPipeline->m_RootSignature;

		SPALL_TRY(requireResourceSetStates(rootSignature));
		SPALL_TRY(m_StateTracker.commitBarriers());
		SPALL_TRY(applyResourceSets(rootSignature, PipelineType::RayTracing));

		applyPushConstants(rootSignature, PipelineType::RayTracing);

		return {};
	}

	Status CommandList::dispatchRays(
		std::uint32_t width,
		std::uint32_t height,
		std::uint32_t depth)
	{
		SPALL_TRY(prepareRayDispatch());

		D3D12_DISPATCH_RAYS_DESC description = m_RayTracingPipeline->m_DispatchDescription;
		description.Width = width;
		description.Height = height;
		description.Depth = depth;

		m_RayTracingCommandList->DispatchRays(&description);

		return {};
	}

	Status CommandList::recordIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset,
		std::uint32_t argumentSize,
		ID3D12CommandSignature& commandSignature,
		IndirectKind kind)
	{
		Buffer* backendBuffer = backendCast<Buffer>(argumentBuffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateIndirectArguments(argumentBuffer, offset, argumentSize));

		Status error = requireBufferState(*backendBuffer, ResourceStateFlags::IndirectArgument);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = (kind == IndirectKind::Dispatch) ? prepareDispatch() : prepareDraw(kind == IndirectKind::DrawIndexed);

		if (error != SUCCESS)
		{
			return error;
		}

		retainResource(argumentBuffer);

		m_CommandList->ExecuteIndirect(&commandSignature, 1, backendBuffer->m_Resource.Get(), offset, nullptr, 0);

		return {};
	}

	Status CommandList::drawIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		return recordIndirect(
			argumentBuffer,
			offset,
			sizeof(DrawIndirectCommand),
			*m_Device->m_DrawSignature.Get(),
			IndirectKind::Draw);
	}

	Status CommandList::drawIndexedIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		return recordIndirect(
			argumentBuffer,
			offset,
			sizeof(DrawIndexedIndirectCommand),
			*m_Device->m_DrawIndexedSignature.Get(),
			IndirectKind::DrawIndexed);
	}

	Status CommandList::dispatchIndirect(
		IBuffer& argumentBuffer,
		std::uint32_t offset)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		return recordIndirect(
			argumentBuffer,
			offset,
			sizeof(DispatchIndirectCommand),
			*m_Device->m_DispatchSignature.Get(),
			IndirectKind::Dispatch);
	}

	Status CommandList::buildAccelerationStructure(
		IAccelerationStructure& accelerationStructure,
		const AccelerationStructureBuildInfo& buildInfo)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if ((not m_Device->m_RayTracingDevice) or (not m_RayTracingCommandList))
		{
			return ERR_UNSUPPORTED;
		}

		if (m_CommandListType != D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			return ERR_UNSUPPORTED;
		}

		AccelerationStructure* structure = backendCast<AccelerationStructure>(accelerationStructure);

		if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		SPALL_TRY(validateAccelerationStructureBuildInfo(structure->m_Info, buildInfo));

		if (buildInfo.Update and (not structure->m_Built))
		{
			return ERR_INVALID_STATE;
		}

		if (structure->m_Compacted)
		{
			return ERR_INVALID_STATE;
		}

		for (const Resource<Buffer>& inputBuffer : structure->m_InputBuffers)
		{
			Status stateError = requireBufferState(*inputBuffer, ResourceStateFlags::ShaderResource);

			if (stateError != SUCCESS)
			{
				return fail(stateError);
			}
		}

		Status barrierError = m_StateTracker.commitBarriers();

		if (barrierError != SUCCESS)
		{
			return fail(barrierError);
		}

		const bool topLevel = (structure->m_Info.Type == AccelerationStructureType::TopLevel);

		const std::uint32_t instanceCount = (buildInfo.InstanceCount != 0)
			? buildInfo.InstanceCount
			: structure->m_Info.InstanceCount;

		if (topLevel and buildInfo.Update and (instanceCount != structure->m_BuiltInstanceCount))
		{
			return ERR_INVALID_RANGE;
		}

		const D3D12_GPU_VIRTUAL_ADDRESS instanceAddress = topLevel
			? (structure->m_InstanceBuffer->m_Resource->GetGPUVirtualAddress() + structure->m_InstanceBufferOffset)
			: 0;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = accelerationStructureInputs(
			structure->m_Info.Type,
			structure->m_Info.Flags,
			structure->m_GeometryDescriptions,
			instanceCount,
			instanceAddress);

		if (buildInfo.Update)
		{
			inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}

		std::uint64_t scratchSize = buildInfo.Update
			? structure->m_Info.UpdateScratchSize
			: structure->m_Info.BuildScratchSize;

		if (scratchSize < D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT)
		{
			scratchSize = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		}

		ID3D12Resource* scratchBuffer = nullptr;
		Status scratchError = createScratchBuffer(scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &scratchBuffer);

		if (scratchError != SUCCESS)
		{
			return fail(scratchError);
		}

		const D3D12_GPU_VIRTUAL_ADDRESS destinationAddress = structure->m_Resource->GetGPUVirtualAddress();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = structure->m_Resource.Get();

		m_CommandList->ResourceBarrier(1, &barrier);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC description = {};
		description.DestAccelerationStructureData = destinationAddress;
		description.Inputs = inputs;
		description.SourceAccelerationStructureData = buildInfo.Update ? destinationAddress : 0;
		description.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();

		m_RayTracingCommandList->BuildRaytracingAccelerationStructure(&description, 0, nullptr);

		m_CommandList->ResourceBarrier(1, &barrier);

		if (structure->m_CompactedSizeBuffer)
		{
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildDesc = {};
			postbuildDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
			postbuildDesc.DestBuffer = structure->m_CompactedSizeBuffer->GetGPUVirtualAddress();

			m_RayTracingCommandList->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildDesc, 1, &destinationAddress);

			D3D12_RESOURCE_BARRIER sizeBarrier = {};
			sizeBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			sizeBarrier.Transition.pResource = structure->m_CompactedSizeBuffer.Get();
			sizeBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			sizeBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			sizeBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

			m_CommandList->ResourceBarrier(1, &sizeBarrier);

			m_CommandList->CopyBufferRegion(
				structure->m_CompactedSizeReadback.Get(),
				0,
				structure->m_CompactedSizeBuffer.Get(),
				0,
				sizeof(std::uint64_t));

			std::swap(sizeBarrier.Transition.StateBefore, sizeBarrier.Transition.StateAfter);

			m_CommandList->ResourceBarrier(1, &sizeBarrier);
		}

		retainResource(accelerationStructure);
		structure->m_Built = true;
		structure->m_BuiltInstanceCount = instanceCount;

		return {};
	}

	Status CommandList::compactAccelerationStructure(
		IAccelerationStructure& accelerationStructure)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if ((not m_Device->m_RayTracingDevice) or (not m_RayTracingCommandList))
		{
			return ERR_UNSUPPORTED;
		}

		if (m_CommandListType != D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			return ERR_UNSUPPORTED;
		}

		AccelerationStructure* structure = backendCast<AccelerationStructure>(accelerationStructure);

		if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		SPALL_TRY(validateAccelerationStructureCompaction(structure->m_Info));

		if ((not structure->m_Built) or structure->m_Compacted)
		{
			return ERR_INVALID_STATE;
		}

		std::uint64_t compactedSize = 0;
		void* mapped = nullptr;
		const D3D12_RANGE readRange = {0, sizeof(compactedSize)};
		const HRESULT mapResult = structure->m_CompactedSizeReadback->Map(0, &readRange, &mapped);

		if (FAILED(mapResult))
		{
			return mapStatus(mapResult);
		}

		std::memcpy(&compactedSize, mapped, sizeof(compactedSize));

		const D3D12_RANGE writtenRange = {0, 0};
		structure->m_CompactedSizeReadback->Unmap(0, &writtenRange);

		if ((compactedSize == 0) or (compactedSize > structure->m_Info.Size))
		{
			return ERR_BACKEND_FAILURE;
		}

		D3D12_RESOURCE_DESC resourceDesc = structure->m_Resource->GetDesc();
		resourceDesc.Width = compactedSize;

		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_DEFAULT);

		ComPtr<ID3D12Resource> compacted;
		const HRESULT hr = m_Device->m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&compacted));

		if (FAILED(hr))
		{
			return fail(mapStatus(hr));
		}

		m_RayTracingCommandList->CopyRaytracingAccelerationStructure(
			compacted->GetGPUVirtualAddress(),
			structure->m_Resource->GetGPUVirtualAddress(),
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = compacted.Get();

		m_CommandList->ResourceBarrier(1, &barrier);

		m_RetiredAccelerationStructures.push_back(std::move(structure->m_Resource));

		structure->m_Resource = std::move(compacted);
		structure->m_Info.Size = compactedSize;
		structure->m_Compacted = true;

		retainResource(accelerationStructure);

		return {};
	}

	Status CommandList::copyBuffer(
		IBuffer& destination,
		std::uint32_t destinationOffset,
		IBuffer& source,
		std::uint32_t sourceOffset,
		std::uint32_t size)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Buffer* destinationBuffer = backendCast<Buffer>(destination);
		Buffer* sourceBuffer = backendCast<Buffer>(source);

		if ((destinationBuffer == nullptr) or (sourceBuffer == nullptr))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((destinationBuffer->m_Device.get() != m_Device.get()) or (sourceBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateCopyBufferArguments(destination, destinationOffset, source, sourceOffset, size));

		Status error = requireBufferState(*destinationBuffer, ResourceStateFlags::CopyDest);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireBufferState(*sourceBuffer, ResourceStateFlags::CopySource);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(destination);
		retainResource(source);

		m_CommandList->CopyBufferRegion(
			destinationBuffer->m_Resource.Get(),
			destinationOffset,
			sourceBuffer->m_Resource.Get(),
			sourceOffset,
			size);

		return {};
	}

	Status CommandList::copyBufferToTexture(
		ITexture& destination,
		const TextureRegion& region,
		IBuffer& source,
		std::uint32_t sourceOffset,
		std::uint32_t sourceRowPitch)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Texture* destinationTexture = backendCast<Texture>(destination);
		Buffer* sourceBuffer = backendCast<Buffer>(source);

		if ((destinationTexture == nullptr) or (sourceBuffer == nullptr))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((destinationTexture->m_Device.get() != m_Device.get()) or (sourceBuffer->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((destinationTexture->m_Info.Usage & TextureUsageFlags::TransferDestination) == TextureUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		SPALL_TRY(validateTextureBufferCopyArguments(destination, region, source, sourceOffset, sourceRowPitch));

		const TextureRegion resolved = resolveTextureRegion(destinationTexture->m_Info, region);
		const RegionLayout layout = regionLayout(destinationTexture->m_Info.Format, resolved);

		Status error = requireTextureState(
			*destinationTexture,
			ResourceStateFlags::CopyDest,
			TextureSubresourceRange {resolved.MipLevel, 1, resolved.ArrayLayer, 1});

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireBufferState(*sourceBuffer, ResourceStateFlags::CopySource);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(destination);
		retainResource(source);

		ID3D12Resource* footprintResource = sourceBuffer->m_Resource.Get();
		std::uint64_t footprintOffset = sourceOffset;
		std::uint32_t footprintRowPitch = sourceRowPitch;

		const bool aligned = ((sourceOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) == 0) and
			((sourceRowPitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) == 0);

		const std::uint32_t copyRows = layout.RowCount * resolved.Depth;

		if (not aligned)
		{
			const std::uint64_t alignedRowPitch = Alignment::up(layout.RowBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			const std::uint64_t scratchSize = (alignedRowPitch * (copyRows - 1)) + layout.RowBytes;

			ID3D12Resource* scratchBuffer = nullptr;
			error = createScratchBuffer(scratchSize, D3D12_RESOURCE_FLAG_NONE, &scratchBuffer);

			if (error != SUCCESS)
			{
				return fail(error);
			}

			transitionScratchBuffer(*scratchBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			for (std::uint32_t row = 0; row < copyRows; ++row)
			{
				m_CommandList->CopyBufferRegion(
					scratchBuffer,
					alignedRowPitch * row,
					sourceBuffer->m_Resource.Get(),
					sourceOffset + (static_cast<std::uint64_t>(sourceRowPitch) * row),
					layout.RowBytes);
			}

			transitionScratchBuffer(*scratchBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);

			footprintResource = scratchBuffer;
			footprintOffset = 0;
			footprintRowPitch = static_cast<std::uint32_t>(alignedRowPitch);
		}

		D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
		destinationLocation.pResource = destinationTexture->m_Resource.Get();
		destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destinationLocation.SubresourceIndex = subresourceIndex(destinationTexture->m_Info, resolved.MipLevel, resolved.ArrayLayer);

		D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
		sourceLocation.pResource = footprintResource;
		sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		sourceLocation.PlacedFootprint.Offset = footprintOffset;
		sourceLocation.PlacedFootprint.Footprint.Format = format(destinationTexture->m_Info.Format);
		sourceLocation.PlacedFootprint.Footprint.Width = layout.FootprintWidth;
		sourceLocation.PlacedFootprint.Footprint.Height = layout.FootprintHeight;
		sourceLocation.PlacedFootprint.Footprint.Depth = resolved.Depth;
		sourceLocation.PlacedFootprint.Footprint.RowPitch = footprintRowPitch;

		D3D12_BOX sourceBox = {};
		sourceBox.left = 0;
		sourceBox.top = 0;
		sourceBox.front = 0;
		sourceBox.right = layout.FootprintWidth;
		sourceBox.bottom = layout.FootprintHeight;
		sourceBox.back = resolved.Depth;

		m_CommandList->CopyTextureRegion(&destinationLocation, resolved.X, resolved.Y, resolved.Z, &sourceLocation, &sourceBox);

		return {};
	}

	Status CommandList::copyTextureToBuffer(
		IBuffer& destination,
		std::uint32_t destinationOffset,
		std::uint32_t destinationRowPitch,
		ITexture& source,
		const TextureRegion& region)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Buffer* destinationBuffer = backendCast<Buffer>(destination);
		Texture* sourceTexture = backendCast<Texture>(source);

		if ((destinationBuffer == nullptr) or (sourceTexture == nullptr))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((destinationBuffer->m_Device.get() != m_Device.get()) or (sourceTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((sourceTexture->m_Info.Usage & TextureUsageFlags::TransferSource) == TextureUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((destinationBuffer->m_Info.Usage & BufferUsageFlags::TransferDestination) == BufferUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		SPALL_TRY(validateTextureBufferCopyArguments(source, region, destination, destinationOffset, destinationRowPitch));

		const TextureRegion resolved = resolveTextureRegion(sourceTexture->m_Info, region);
		const RegionLayout layout = regionLayout(sourceTexture->m_Info.Format, resolved);

		Status error = requireTextureState(
			*sourceTexture,
			ResourceStateFlags::CopySource,
			TextureSubresourceRange {resolved.MipLevel, 1, resolved.ArrayLayer, 1});

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireBufferState(*destinationBuffer, ResourceStateFlags::CopyDest);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(destination);
		retainResource(source);

		const std::uint32_t copyRows = layout.RowCount * resolved.Depth;

		const bool aligned = ((destinationOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) == 0) and
			((destinationRowPitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) == 0);

		ID3D12Resource* footprintResource = destinationBuffer->m_Resource.Get();
		std::uint64_t footprintOffset = destinationOffset;
		std::uint64_t footprintRowPitch = destinationRowPitch;
		ID3D12Resource* scratchBuffer = nullptr;

		if (not aligned)
		{
			footprintRowPitch = Alignment::up(layout.RowBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			const std::uint64_t scratchSize = (footprintRowPitch * (copyRows - 1)) + layout.RowBytes;

			error = createScratchBuffer(scratchSize, D3D12_RESOURCE_FLAG_NONE, &scratchBuffer);

			if (error != SUCCESS)
			{
				return fail(error);
			}

			transitionScratchBuffer(*scratchBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			footprintResource = scratchBuffer;
			footprintOffset = 0;
		}

		D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
		destinationLocation.pResource = footprintResource;
		destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destinationLocation.PlacedFootprint.Offset = footprintOffset;
		destinationLocation.PlacedFootprint.Footprint.Format = format(sourceTexture->m_Info.Format);
		destinationLocation.PlacedFootprint.Footprint.Width = layout.FootprintWidth;
		destinationLocation.PlacedFootprint.Footprint.Height = layout.FootprintHeight;
		destinationLocation.PlacedFootprint.Footprint.Depth = resolved.Depth;
		destinationLocation.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(footprintRowPitch);

		D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
		sourceLocation.pResource = sourceTexture->m_Resource.Get();
		sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		sourceLocation.SubresourceIndex = subresourceIndex(sourceTexture->m_Info, resolved.MipLevel, resolved.ArrayLayer);

		D3D12_BOX sourceBox = {};
		sourceBox.left = resolved.X;
		sourceBox.top = resolved.Y;
		sourceBox.front = resolved.Z;
		sourceBox.right = resolved.X + layout.FootprintWidth;
		sourceBox.bottom = resolved.Y + layout.FootprintHeight;
		sourceBox.back = resolved.Z + resolved.Depth;

		m_CommandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, &sourceBox);

		if (scratchBuffer != nullptr)
		{
			transitionScratchBuffer(*scratchBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);

			for (std::uint32_t row = 0; row < copyRows; ++row)
			{
				m_CommandList->CopyBufferRegion(
					destinationBuffer->m_Resource.Get(),
					destinationOffset + (static_cast<std::uint64_t>(destinationRowPitch) * row),
					scratchBuffer,
					footprintRowPitch * row,
					layout.RowBytes);
			}
		}

		return {};
	}

	Status CommandList::generateMips(
		ITexture& texture)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Texture* backendTexture = backendCast<Texture>(texture);

		if (backendTexture == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendTexture->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateGenerateMipsArguments(texture));

		const TextureInfo& info = backendTexture->m_Info;

		ID3D12RootSignature* rootSignature = nullptr;
		ID3D12PipelineState* pipelineState = nullptr;
		SPALL_TRY(m_Device->m_MipmapGenerator.pipelineState(*m_Device, info.Format, &rootSignature, &pipelineState));

		ComPtr<ID3D12Resource> scratch;
		SPALL_TRY(createMipmapScratchTexture(info, &scratch));

		m_CommandList->SetGraphicsRootSignature(rootSignature);
		m_CommandList->SetPipelineState(pipelineState);
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		Status error = requireTextureState(
			*backendTexture,
			ResourceStateFlags::ShaderResource,
			TextureSubresourceRange {0, 1, 0, info.ArrayLayers});

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireTextureState(
			*backendTexture,
			ResourceStateFlags::CopyDest,
			TextureSubresourceRange {1, info.MipLevels - 1, 0, info.ArrayLayers});

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(texture);

		for (std::uint32_t arrayLayer = 0; arrayLayer < info.ArrayLayers; ++arrayLayer)
		{
			for (std::uint32_t mipLevel = 1; mipLevel < info.MipLevels; ++mipLevel)
			{
				const bool sourceIsScratch = (mipLevel > 1);
				ID3D12Resource* sourceResource = sourceIsScratch ? scratch.Get() : backendTexture->m_Resource.Get();

				if (sourceIsScratch)
				{
					transitionScratchSubresource(
						*scratch.Get(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						subresourceIndex(info, mipLevel - 1, arrayLayer));
				}

				D3D12_CPU_DESCRIPTOR_HANDLE sourceDescriptor = {};
				D3D12_GPU_DESCRIPTOR_HANDLE sourceTable = {};
				error = m_Rings.Views.allocate(1, &sourceDescriptor, &sourceTable);

				if (error != SUCCESS)
				{
					return fail(error);
				}

				D3D12_SHADER_RESOURCE_VIEW_DESC sourceViewDesc = {};
				sourceViewDesc.Format = format(info.Format);
				sourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				sourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				sourceViewDesc.Texture2DArray.MostDetailedMip = mipLevel - 1;
				sourceViewDesc.Texture2DArray.MipLevels = 1;
				sourceViewDesc.Texture2DArray.FirstArraySlice = arrayLayer;
				sourceViewDesc.Texture2DArray.ArraySize = 1;

				m_Device->m_Device->CreateShaderResourceView(sourceResource, &sourceViewDesc, sourceDescriptor);

				std::uint32_t renderTargetIndex = InvalidDescriptorIndex;
				error = m_Device->m_RenderTargetViews.allocate(&renderTargetIndex);

				if (error != SUCCESS)
				{
					return fail(error);
				}

				D3D12_RENDER_TARGET_VIEW_DESC targetViewDesc = {};
				targetViewDesc.Format = format(info.Format);
				targetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				targetViewDesc.Texture2DArray.MipSlice = mipLevel;
				targetViewDesc.Texture2DArray.FirstArraySlice = arrayLayer;
				targetViewDesc.Texture2DArray.ArraySize = 1;

				const D3D12_CPU_DESCRIPTOR_HANDLE targetDescriptor = m_Device->m_RenderTargetViews.cpuHandle(renderTargetIndex);
				m_Device->m_Device->CreateRenderTargetView(scratch.Get(), &targetViewDesc, targetDescriptor);

				const std::uint32_t targetWidth = mipLevelExtent(info.Width, mipLevel);
				const std::uint32_t targetHeight = mipLevelExtent(info.Height, mipLevel);

				D3D12_VIEWPORT viewport = {};
				viewport.Width = static_cast<float>(targetWidth);
				viewport.Height = static_cast<float>(targetHeight);
				viewport.MaxDepth = 1.0f;

				D3D12_RECT scissor = {};
				scissor.right = static_cast<LONG>(targetWidth);
				scissor.bottom = static_cast<LONG>(targetHeight);

				m_CommandList->RSSetViewports(1, &viewport);
				m_CommandList->RSSetScissorRects(1, &scissor);
				m_CommandList->OMSetRenderTargets(1, &targetDescriptor, FALSE, nullptr);
				m_CommandList->SetGraphicsRootDescriptorTable(0, sourceTable);
				m_CommandList->DrawInstanced(3, 1, 0, 0);

				m_Device->m_RenderTargetViews.release(renderTargetIndex);
			}
		}

		for (std::uint32_t arrayLayer = 0; arrayLayer < info.ArrayLayers; ++arrayLayer)
		{
			for (std::uint32_t mipLevel = 1; mipLevel < info.MipLevels; ++mipLevel)
			{
				const D3D12_RESOURCE_STATES scratchState = (mipLevel == (info.MipLevels - 1))
					? D3D12_RESOURCE_STATE_RENDER_TARGET
					: D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

				transitionScratchSubresource(
					*scratch.Get(),
					scratchState,
					D3D12_RESOURCE_STATE_COPY_SOURCE,
					subresourceIndex(info, mipLevel, arrayLayer));

				D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
				destinationLocation.pResource = backendTexture->m_Resource.Get();
				destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				destinationLocation.SubresourceIndex = subresourceIndex(info, mipLevel, arrayLayer);

				D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
				sourceLocation.pResource = scratch.Get();
				sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				sourceLocation.SubresourceIndex = subresourceIndex(info, mipLevel, arrayLayer);

				m_CommandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
			}
		}

		m_GraphicsPipeline = nullptr;
		m_ComputePipeline = nullptr;
		m_ViewportSet = false;
		m_ScissorSet = false;

		return {};
	}

	Status CommandList::copyTexture(
		ITexture& destination,
		ITexture& source)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		if (m_RenderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		Texture* destinationTexture = backendCast<Texture>(destination);
		Texture* sourceTexture = backendCast<Texture>(source);

		if ((destinationTexture == nullptr) or (sourceTexture == nullptr))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((destinationTexture->m_Device.get() != m_Device.get()) or (sourceTexture->m_Device.get() != m_Device.get()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateCopyTextureArguments(destination, source));

		Status error = requireTextureState(*destinationTexture, ResourceStateFlags::CopyDest);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = requireTextureState(*sourceTexture, ResourceStateFlags::CopySource);

		if (error != SUCCESS)
		{
			return fail(error);
		}

		error = m_StateTracker.commitBarriers();

		if (error != SUCCESS)
		{
			return fail(error);
		}

		retainResource(destination);
		retainResource(source);

		m_CommandList->CopyResource(destinationTexture->m_Resource.Get(), sourceTexture->m_Resource.Get());

		return {};
	}

	Status CommandList::writeTimestamp(
		IQueryPool& queryPool,
		std::uint32_t query)
	{
		SPALL_TRY(spall::validateRecordingState(m_ExecutionState == ExecutionState::Recording, true));

		QueryPool* backendQueryPool = backendCast<QueryPool>(queryPool);

		if (backendQueryPool == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendQueryPool->m_Device.get() != m_Device.get())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateTimestampWrite(backendQueryPool->m_Info, query));

		m_CommandList->EndQuery(backendQueryPool->m_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, query);
		m_TimestampWrites.emplace_back(Resource<QueryPool>(backendQueryPool), query);

		return {};
	}

	void CommandList::resolveTimestampWrites()
	{
		for (std::size_t writeIndex = 0; writeIndex < m_TimestampWrites.size(); ++writeIndex)
		{
			QueryPool* queryPool = m_TimestampWrites[writeIndex].first.get();
			bool alreadyResolved = false;

			for (std::size_t earlierIndex = 0; earlierIndex < writeIndex; ++earlierIndex)
			{
				if (m_TimestampWrites[earlierIndex].first.get() == queryPool)
				{
					alreadyResolved = true;
					break;
				}
			}

			if (alreadyResolved)
			{
				continue;
			}

			m_CommandList->ResolveQueryData(
				queryPool->m_QueryHeap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				0,
				queryPool->m_Info.TimestampCount,
				queryPool->m_ResultBuffer.Get(),
				0);
		}
	}
} // namespace spall::d3d12
