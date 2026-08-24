// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <spall/Common/Assert.h>
#include <spall/Common/Limits.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_HeapMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_PipelineMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_RayTracingMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_ResourceStateMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_SamplerMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_TextureUsageMappings.h>
#include <src/Backends/D3D12/Framebuffer/D3D12_Framebuffer.h>
#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>
#include <src/Backends/D3D12/Resources/AccelerationStructure/D3D12_AccelerationStructure.h>
#include <src/Backends/D3D12/Resources/Buffer/D3D12_Buffer.h>
#include <src/Backends/D3D12/Resources/Query/D3D12_QueryPool.h>
#include <src/Backends/D3D12/Resources/Sampler/D3D12_Sampler.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <utility>

namespace spall::d3d12
{
	Status Device::copyBufferImmediate(
		ID3D12Resource& destination,
		D3D12_RESOURCE_STATES destinationState,
		ID3D12Resource& source,
		std::uint64_t size)
	{
		HRESULT hr = S_OK;

		if (not m_UploadCommandAllocator)
		{
			hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_UploadCommandAllocator));

			if (FAILED(hr))
			{
				return mapStatus(hr);
			}

			hr = m_Device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				m_UploadCommandAllocator.Get(),
				nullptr,
				IID_PPV_ARGS(&m_UploadCommandList));

			if (FAILED(hr))
			{
				return mapStatus(hr);
			}
		}
		else
		{
			hr = m_UploadCommandAllocator->Reset();

			if (FAILED(hr))
			{
				return mapStatus(hr);
			}

			hr = m_UploadCommandList->Reset(m_UploadCommandAllocator.Get(), nullptr);

			if (FAILED(hr))
			{
				return mapStatus(hr);
			}
		}

		ID3D12GraphicsCommandList* const commandList = m_UploadCommandList.Get();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = &destination;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		const bool transitionNeeded = (destinationState != D3D12_RESOURCE_STATE_COPY_DEST);

		if (transitionNeeded)
		{
			barrier.Transition.StateBefore = destinationState;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			commandList->ResourceBarrier(1, &barrier);
		}

		commandList->CopyBufferRegion(&destination, 0, &source, 0, size);

		if (transitionNeeded)
		{
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = destinationState;
			commandList->ResourceBarrier(1, &barrier);
		}

		hr = commandList->Close();

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		ID3D12CommandList* const submittedCommandLists[] = {commandList};
		m_CommandQueue->ExecuteCommandLists(1, submittedCommandLists);

		std::uint64_t fenceValue = 0;
		SPALL_TRY(m_GraphicsQueue->signal(&fenceValue));

		return m_GraphicsQueue->waitForFenceValue(fenceValue);
	}

	Status Device::createTexture(
		const TextureCreateInfo& info,
		Resource<ITexture>* texture)
	{
		if (texture == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateTextureCreateInfo(info));

		const DXGI_FORMAT textureFormat = format(info.Format);

		if (textureFormat == DXGI_FORMAT_UNKNOWN)
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		if ((info.Width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) or
			(info.Height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) or
			(info.ArrayLayers > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.Cubemap and (info.Width > D3D12_REQ_TEXTURECUBE_DIMENSION))
		{
			return ERR_INVALID_SIZE;
		}

		D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};
		formatSupport.Format = textureFormat;
		HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		const D3D12_FORMAT_SUPPORT1 required = requiredFormatSupport(info.Usage);

		if ((formatSupport.Support1 & required) != required)
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		if (info.SampleCount > 1)
		{
			D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {};
			qualityLevels.Format = textureFormat;
			qualityLevels.SampleCount = info.SampleCount;

			hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels));

			if (FAILED(hr) or (qualityLevels.NumQualityLevels == 0))
			{
				return ERR_UNSUPPORTED_USAGE;
			}
		}

		const bool volume = (info.Depth > 1);

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = volume ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = info.Width;
		resourceDesc.Height = info.Height;
		resourceDesc.DepthOrArraySize = static_cast<UINT16>(volume ? info.Depth : info.ArrayLayers);
		resourceDesc.MipLevels = static_cast<UINT16>(info.MipLevels);
		resourceDesc.Format = textureFormat;
		resourceDesc.SampleDesc.Count = info.SampleCount;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = textureUsageFlags(info.Usage);

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = textureFormat;
		const bool depthStencil = ((info.Usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None);

		if (depthStencil)
		{
			clearValue.DepthStencil.Depth = 1.0f;
			clearValue.DepthStencil.Stencil = 0;
		}

		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_STATES initialState = resourceState(info.InitialState);

		ComPtr<ID3D12Resource> resource;
		hr = m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState,
			depthStencil ? &clearValue : nullptr,
			IID_PPV_ARGS(&resource));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		TextureInfo textureInfo = {};
		textureInfo.Width = info.Width;
		textureInfo.Height = info.Height;
		textureInfo.Depth = info.Depth;
		textureInfo.MipLevels = info.MipLevels;
		textureInfo.ArrayLayers = info.ArrayLayers;
		textureInfo.SampleCount = info.SampleCount;
		textureInfo.Cubemap = info.Cubemap;
		textureInfo.Format = info.Format;
		textureInfo.Usage = info.Usage;
		textureInfo.InitialState = info.InitialState;
		textureInfo.KeepInitialState = info.KeepInitialState;
		textureInfo.DebugName = info.DebugName;

		*texture = Resource<ITexture>(new Texture(*this, textureInfo, std::move(resource)));

		return {};
	}

	Status Device::createTextureView(
		const TextureViewCreateInfo& info,
		Resource<ITextureView>* textureView)
	{
		if (textureView == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateTextureViewCreateInfo(info));

		Texture* texture = backendCast<Texture>(info.Texture);

		if (texture == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (texture->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		const Format format = (info.Format == Format::Unknown) ? texture->m_Info.Format : info.Format;
		const DXGI_FORMAT viewFormat = d3d12::format(format);

		if (viewFormat == DXGI_FORMAT_UNKNOWN)
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		TextureView::Subresources subresources = {};
		subresources.BaseMipLevel = info.BaseMipLevel;
		subresources.MipLevels = (info.MipLevels != 0) ? info.MipLevels : (texture->m_Info.MipLevels - info.BaseMipLevel);
		subresources.BaseArrayLayer = info.BaseArrayLayer;
		subresources.ArrayLayers = (info.ArrayLayers != 0) ? info.ArrayLayers : (texture->m_Info.ArrayLayers - info.BaseArrayLayer);
		subresources.Cubemap = info.Cubemap;
		subresources.Aspects = info.Aspects;

		if (subresources.Aspects == TextureAspectFlags::None)
		{
			subresources.Aspects = hasStencilAspect(format)
				? (TextureAspectFlags::Depth | TextureAspectFlags::Stencil)
				: (isDepthFormat(format) ? TextureAspectFlags::Depth : TextureAspectFlags::Color);
		}

		const bool layered = (subresources.ArrayLayers > 1) or (info.BaseArrayLayer != 0);
		const bool multisampled = (texture->m_Info.SampleCount > 1);
		const bool volume = (texture->m_Info.Depth > 1);

		std::uint32_t renderTargetViewIndex = InvalidDescriptorIndex;
		std::uint32_t depthStencilViewIndex = InvalidDescriptorIndex;

		if (((subresources.Aspects & TextureAspectFlags::Color) != TextureAspectFlags::None) and
			((texture->m_Info.Usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None))
		{
			SPALL_TRY(m_RenderTargetViews.allocate(&renderTargetViewIndex));

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = viewFormat;

			if (volume)
			{
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				viewDesc.Texture3D.MipSlice = info.BaseMipLevel;
				viewDesc.Texture3D.FirstWSlice = 0;
				viewDesc.Texture3D.WSize = mipLevelExtent(texture->m_Info.Depth, info.BaseMipLevel);
			}
			else if (multisampled and layered)
			{
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
				viewDesc.Texture2DMSArray.FirstArraySlice = info.BaseArrayLayer;
				viewDesc.Texture2DMSArray.ArraySize = subresources.ArrayLayers;
			}
			else if (multisampled)
			{
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			}
			else if (layered)
			{
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				viewDesc.Texture2DArray.MipSlice = info.BaseMipLevel;
				viewDesc.Texture2DArray.FirstArraySlice = info.BaseArrayLayer;
				viewDesc.Texture2DArray.ArraySize = subresources.ArrayLayers;
				viewDesc.Texture2DArray.PlaneSlice = 0;
			}
			else
			{
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = info.BaseMipLevel;
				viewDesc.Texture2D.PlaneSlice = 0;
			}

			m_Device->CreateRenderTargetView(
				texture->m_Resource.Get(),
				&viewDesc,
				m_RenderTargetViews.cpuHandle(renderTargetViewIndex));
		}

		if ((subresources.Aspects & (TextureAspectFlags::Depth | TextureAspectFlags::Stencil)) != TextureAspectFlags::None)
		{
			if ((texture->m_Info.Usage & TextureUsageFlags::DepthStencilAttachment) == TextureUsageFlags::None)
			{
				m_RenderTargetViews.release(renderTargetViewIndex);

				return ERR_INVALID_USAGE_FLAGS;
			}

			Status error = m_DepthStencilViews.allocate(&depthStencilViewIndex);

			if (error != SUCCESS)
			{
				m_RenderTargetViews.release(renderTargetViewIndex);

				return error;
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
			viewDesc.Format = viewFormat;
			viewDesc.Flags = D3D12_DSV_FLAG_NONE;

			if (multisampled and layered)
			{
				viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
				viewDesc.Texture2DMSArray.FirstArraySlice = info.BaseArrayLayer;
				viewDesc.Texture2DMSArray.ArraySize = subresources.ArrayLayers;
			}
			else if (multisampled)
			{
				viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			}
			else if (layered)
			{
				viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				viewDesc.Texture2DArray.MipSlice = info.BaseMipLevel;
				viewDesc.Texture2DArray.FirstArraySlice = info.BaseArrayLayer;
				viewDesc.Texture2DArray.ArraySize = subresources.ArrayLayers;
			}
			else
			{
				viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = info.BaseMipLevel;
			}

			m_Device->CreateDepthStencilView(
				texture->m_Resource.Get(),
				&viewDesc,
				m_DepthStencilViews.cpuHandle(depthStencilViewIndex));
		}

		*textureView = Resource<ITextureView>(
			new TextureView(*texture, subresources, renderTargetViewIndex, depthStencilViewIndex));

		return {};
	}

	Status Device::createFramebuffer(
		const FramebufferCreateInfo& createInfo,
		Resource<IFramebuffer>* framebuffer)
	{
		if (framebuffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateFramebufferCreateInfo(createInfo));

		TextureView* colorViews[MaxColorAttachments] = {};
		FramebufferInfo info = {};
		info.ColorFormatCount = createInfo.ColorAttachmentCount;
		info.SampleCount = framebufferSampleCount(createInfo);

		bool haveDimensions = false;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < createInfo.ColorAttachmentCount; ++attachmentIndex)
		{
			TextureView* colorView = backendCast<TextureView>(createInfo.ColorAttachments[attachmentIndex]);

			if ((colorView == nullptr) or (not colorView->m_Texture) or (colorView->m_Texture->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			if (colorView->m_RenderTargetViewIndex == InvalidDescriptorIndex)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			const std::uint32_t width = mipLevelExtent(colorView->m_Texture->m_Info.Width, colorView->m_BaseMipLevel);
			const std::uint32_t height = mipLevelExtent(colorView->m_Texture->m_Info.Height, colorView->m_BaseMipLevel);

			if (not haveDimensions)
			{
				info.Width = width;
				info.Height = height;
				haveDimensions = true;
			}
			else if ((info.Width != width) or (info.Height != height))
			{
				return ERR_INVALID_RESOURCE;
			}

			colorViews[attachmentIndex] = colorView;
			info.ColorFormats[attachmentIndex] = colorView->m_Texture->m_Info.Format;
		}

		TextureView* depthView = nullptr;

		if (createInfo.DepthAttachment != nullptr)
		{
			depthView = backendCast<TextureView>(createInfo.DepthAttachment);

			if ((depthView == nullptr) or (not depthView->m_Texture) or (depthView->m_Texture->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			if (depthView->m_DepthStencilViewIndex == InvalidDescriptorIndex)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			const std::uint32_t width = mipLevelExtent(depthView->m_Texture->m_Info.Width, depthView->m_BaseMipLevel);
			const std::uint32_t height = mipLevelExtent(depthView->m_Texture->m_Info.Height, depthView->m_BaseMipLevel);

			if (not haveDimensions)
			{
				info.Width = width;
				info.Height = height;
				haveDimensions = true;
			}
			else if ((info.Width != width) or (info.Height != height))
			{
				return ERR_INVALID_RESOURCE;
			}

			info.DepthFormat = depthView->m_Texture->m_Info.Format;
		}

		TextureView* resolveViews[MaxColorAttachments] = {};

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < createInfo.ColorAttachmentCount; ++attachmentIndex)
		{
			if (createInfo.ResolveAttachments[attachmentIndex] == nullptr)
			{
				continue;
			}

			TextureView* resolveView = backendCast<TextureView>(createInfo.ResolveAttachments[attachmentIndex]);

			if ((resolveView == nullptr) or (not resolveView->m_Texture) or (resolveView->m_Texture->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			resolveViews[attachmentIndex] = resolveView;
		}

		Framebuffer* createdFramebuffer = new Framebuffer(
			*this,
			info,
			colorViews,
			createInfo.ColorAttachmentCount,
			depthView,
			resolveViews);

		*framebuffer = Resource<IFramebuffer>(createdFramebuffer);

		return {};
	}

	Status Device::createBuffer(
		const BufferCreateInfo& info,
		Resource<IBuffer>* buffer)
	{
		if (buffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateBufferCreateInfo(info));

		if (((info.Usage & BufferUsageFlags::Storage) != BufferUsageFlags::None) and ((info.Size % 4) != 0))
		{
			return ERR_INVALID_SIZE;
		}

		if (((info.Usage & BufferUsageFlags::Uniform) != BufferUsageFlags::None) and
			((info.Size % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) != 0))
		{
			return ERR_INVALID_SIZE;
		}

		const D3D12_HEAP_TYPE heapType = bufferHeapType(info.CpuAccess);

		if ((heapType != D3D12_HEAP_TYPE_DEFAULT) and ((info.Usage & BufferUsageFlags::Storage) != BufferUsageFlags::None))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = info.Size;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = ((info.Usage & BufferUsageFlags::Storage) != BufferUsageFlags::None)
			? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			: D3D12_RESOURCE_FLAG_NONE;

		D3D12_RESOURCE_STATES initialState = resourceState(info.InitialState);

		if (heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (heapType == D3D12_HEAP_TYPE_READBACK)
		{
			initialState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		const D3D12_HEAP_PROPERTIES properties = heapProperties(heapType);

		ComPtr<ID3D12Resource> resource;
		const HRESULT hr = m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState,
			nullptr,
			IID_PPV_ARGS(&resource));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		const BufferInfo bufferInfo {info.Size, info.Usage, info.CpuAccess, info.InitialState, info.KeepInitialState, info.DebugName};

		*buffer = Resource<IBuffer>(new Buffer(*this, bufferInfo, std::move(resource), heapType));

		return {};
	}

	Status Device::createBufferWithData(
		const BufferCreateInfo& info,
		std::span<const std::byte> data,
		Resource<IBuffer>* buffer)
	{
		if (buffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (data.empty())
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (data.size() != info.Size)
		{
			return ERR_INVALID_SIZE;
		}

		Resource<IBuffer> createdBuffer;
		SPALL_TRY(createBuffer(info, &createdBuffer));

		if (info.CpuAccess == MemoryAccess::Write)
		{
			SPALL_TRY(writeBuffer(*createdBuffer, data, 0));

			*buffer = std::move(createdBuffer);

			return {};
		}

		ComPtr<ID3D12Resource> staging;
		SPALL_TRY(m_ResourcePool.acquireBuffer(*this, info.Size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, &staging));

		const D3D12_RANGE readRange = {0, 0};
		void* mappedData = nullptr;
		const HRESULT hr = staging->Map(0, &readRange, &mappedData);

		if (FAILED(hr))
		{
			m_ResourcePool.release(staging);

			return mapStatus(hr);
		}

		std::memcpy(mappedData, data.data(), data.size());

		const D3D12_RANGE writtenRange = {0, data.size()};
		staging->Unmap(0, &writtenRange);

		Buffer* destinationBuffer = backendCast<Buffer>(createdBuffer.get());
		SPALL_ASSERT(destinationBuffer != nullptr);

		const Status copyError = copyBufferImmediate(
			*destinationBuffer->m_Resource.Get(),
			resourceState(info.InitialState),
			*staging.Get(),
			info.Size);

		m_ResourcePool.release(staging);

		SPALL_TRY(copyError);

		*buffer = std::move(createdBuffer);

		return {};
	}

	Status Device::writeBuffer(
		IBuffer& buffer,
		std::span<const std::byte> data,
		std::uint32_t offset)
	{
		if (data.empty())
		{
			return ERR_INVALID_SIZE;
		}

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendBuffer->m_Info.CpuAccess != MemoryAccess::Write)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((offset > backendBuffer->m_Info.Size) or (data.size() > (backendBuffer->m_Info.Size - offset)))
		{
			return ERR_INVALID_RANGE;
		}

		const D3D12_RANGE readRange = {0, 0};
		void* mappedData = nullptr;
		const HRESULT hr = backendBuffer->m_Resource->Map(0, &readRange, &mappedData);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		std::memcpy(static_cast<std::uint8_t*>(mappedData) + offset, data.data(), data.size());

		const D3D12_RANGE writtenRange = {offset, offset + data.size()};
		backendBuffer->m_Resource->Unmap(0, &writtenRange);

		return {};
	}

	Status Device::readBuffer(
		IBuffer& buffer,
		std::span<std::byte> data,
		std::uint32_t offset)
	{
		if (data.empty())
		{
			return ERR_INVALID_SIZE;
		}

		Buffer* backendBuffer = backendCast<Buffer>(buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendBuffer->m_Info.CpuAccess != MemoryAccess::Read)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((offset > backendBuffer->m_Info.Size) or (data.size() > (backendBuffer->m_Info.Size - offset)))
		{
			return ERR_INVALID_RANGE;
		}

		const D3D12_RANGE readRange = {offset, offset + data.size()};
		void* mappedData = nullptr;
		const HRESULT hr = backendBuffer->m_Resource->Map(0, &readRange, &mappedData);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		std::memcpy(data.data(), static_cast<const std::uint8_t*>(mappedData) + offset, data.size());

		const D3D12_RANGE writtenRange = {0, 0};
		backendBuffer->m_Resource->Unmap(0, &writtenRange);

		return {};
	}

	Status Device::createSampler(
		const SamplerCreateInfo& info,
		Resource<ISampler>* sampler)
	{
		if (sampler == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateSamplerCreateInfo(info));

		const bool anisotropic = (info.MaxAnisotropy > 1.0f);

		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = samplerFilter(info.MinFilter, info.MagFilter, info.MipFilter, anisotropic, info.ComparisonEnabled);
		samplerDesc.AddressU = addressMode(info.AddressModeU);
		samplerDesc.AddressV = addressMode(info.AddressModeV);
		samplerDesc.AddressW = addressMode(info.AddressModeW);
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MinLOD = info.MinLod;
		samplerDesc.MaxLOD = info.MaxLod;
		samplerDesc.MaxAnisotropy = anisotropic
			? static_cast<UINT>((std::min)(info.MaxAnisotropy, m_Limits.MaxSamplerAnisotropy))
			: 1;
		samplerDesc.ComparisonFunc = info.ComparisonEnabled
			? compareOp(info.Comparison)
			: D3D12_COMPARISON_FUNC_NEVER;

		std::uint32_t descriptorIndex = InvalidDescriptorIndex;
		SPALL_TRY(m_SamplerDescriptors.allocate(&descriptorIndex));

		m_Device->CreateSampler(&samplerDesc, m_SamplerDescriptors.cpuHandle(descriptorIndex));

		*sampler = Resource<ISampler>(new Sampler(*this, descriptorIndex));

		return {};
	}

	Status Device::createQueryPool(
		const QueryPoolCreateInfo& info,
		Resource<IQueryPool>* queryPool)
	{
		if (queryPool == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateQueryPoolCreateInfo(info));

		if (not m_Limits.SupportsTimestampQueries)
		{
			return ERR_UNSUPPORTED;
		}

		D3D12_QUERY_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		heapDesc.Count = info.TimestampCount;
		heapDesc.NodeMask = 0;

		ComPtr<ID3D12QueryHeap> queryHeap;
		HRESULT hr = m_Device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = static_cast<UINT64>(info.TimestampCount) * sizeof(std::uint64_t);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_READBACK);

		ComPtr<ID3D12Resource> resultBuffer;
		hr = m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&resultBuffer));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		const QueryPoolInfo poolInfo {info.TimestampCount, info.DebugName};

		*queryPool = Resource<IQueryPool>(new QueryPool(*this, poolInfo, std::move(queryHeap), std::move(resultBuffer)));

		return {};
	}

	Status Device::readTimestamps(
		IQueryPool& queryPool,
		std::uint32_t firstQuery,
		std::span<std::uint64_t> nanoseconds)
	{
		QueryPool* backendQueryPool = backendCast<QueryPool>(queryPool);

		if ((backendQueryPool == nullptr) or (backendQueryPool->m_Device.get() != this))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		SPALL_TRY(validateTimestampRead(backendQueryPool->m_Info, firstQuery, nanoseconds.size()));

		const std::uint64_t completedFenceValue = m_GraphicsQueue->completedFenceValue();

		for (std::size_t index = 0; index < nanoseconds.size(); ++index)
		{
			const std::uint64_t writtenFenceValue = backendQueryPool->m_QueryFenceValues[firstQuery + index];

			if ((writtenFenceValue == 0) or (writtenFenceValue > completedFenceValue))
			{
				return ERR_NOT_READY;
			}
		}

		const D3D12_RANGE readRange = {
			static_cast<SIZE_T>(firstQuery) * sizeof(std::uint64_t),
			static_cast<SIZE_T>(firstQuery + nanoseconds.size()) * sizeof(std::uint64_t)};

		void* mappedData = nullptr;
		const HRESULT hr = backendQueryPool->m_ResultBuffer->Map(0, &readRange, &mappedData);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		const std::uint64_t* ticks = static_cast<const std::uint64_t*>(mappedData) + firstQuery;

		for (std::size_t index = 0; index < nanoseconds.size(); ++index)
		{
			nanoseconds[index] = static_cast<std::uint64_t>((static_cast<double>(ticks[index]) * 1000000000.0) / static_cast<double>(m_TimestampFrequency));
		}

		const D3D12_RANGE writtenRange = {0, 0};
		backendQueryPool->m_ResultBuffer->Unmap(0, &writtenRange);

		return {};
	}

	Status Device::createAccelerationStructure(
		const AccelerationStructureCreateInfo& info,
		Resource<IAccelerationStructure>* accelerationStructure)
	{
		if (accelerationStructure == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (not m_RayTracingDevice)
		{
			return ERR_UNSUPPORTED;
		}

		SPALL_TRY(validateAccelerationStructureCreateInfo(info));

		std::vector<Resource<Buffer>> inputBuffers;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescriptions;
		Resource<Buffer> instanceBuffer;

		geometryDescriptions.reserve(info.Geometries.size());

		const auto resolveInput = [this, &inputBuffers](IBuffer* buffer, Buffer** resolved) -> Status
		{
			Buffer* backendBuffer = backendCast<Buffer>(buffer);

			if ((backendBuffer == nullptr) or (backendBuffer->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			if ((backendBuffer->m_Info.Usage & BufferUsageFlags::AccelerationStructureInput) == BufferUsageFlags::None)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			bool retained = false;

			for (const Resource<Buffer>& existing : inputBuffers)
			{
				if (existing.get() == backendBuffer)
				{
					retained = true;
					break;
				}
			}

			if (not retained)
			{
				inputBuffers.push_back(Resource<Buffer>(backendBuffer));
			}

			*resolved = backendBuffer;

			return {};
		};

		for (const AccelerationStructureGeometry& geometry : info.Geometries)
		{
			if (geometry.Type == AccelerationStructureGeometryType::Aabbs)
			{
				Buffer* aabbBuffer = nullptr;
				SPALL_TRY(resolveInput(geometry.AabbBuffer, &aabbBuffer));

				D3D12_RAYTRACING_GEOMETRY_DESC aabbDescription = {};
				aabbDescription.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
				aabbDescription.Flags = accelerationStructureGeometryFlags(geometry.Flags);
				aabbDescription.AABBs.AABBCount = geometry.AabbCount;
				aabbDescription.AABBs.AABBs.StartAddress = aabbBuffer->m_Resource->GetGPUVirtualAddress() + geometry.AabbOffset;
				aabbDescription.AABBs.AABBs.StrideInBytes = geometry.AabbStride;

				geometryDescriptions.push_back(aabbDescription);

				continue;
			}

			if (not isSupportedAccelerationStructureVertexFormat(geometry.VertexFormat))
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			Buffer* vertexBuffer = nullptr;
			SPALL_TRY(resolveInput(geometry.VertexBuffer, &vertexBuffer));

			D3D12_RAYTRACING_GEOMETRY_DESC description = {};
			description.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			description.Flags = accelerationStructureGeometryFlags(geometry.Flags);
			description.Triangles.VertexFormat = format(geometry.VertexFormat);
			description.Triangles.VertexCount = geometry.VertexCount;
			description.Triangles.VertexBuffer.StartAddress = vertexBuffer->m_Resource->GetGPUVirtualAddress() + geometry.VertexOffset;
			description.Triangles.VertexBuffer.StrideInBytes = geometry.VertexStride;

			if (geometry.IndexBuffer != nullptr)
			{
				Buffer* indexBuffer = nullptr;
				SPALL_TRY(resolveInput(geometry.IndexBuffer, &indexBuffer));

				description.Triangles.IndexFormat = indexFormat(geometry.IndexFormat);
				description.Triangles.IndexCount = geometry.IndexCount;
				description.Triangles.IndexBuffer = indexBuffer->m_Resource->GetGPUVirtualAddress() + geometry.IndexOffset;
			}

			if (geometry.TransformBuffer != nullptr)
			{
				Buffer* transformBuffer = nullptr;
				SPALL_TRY(resolveInput(geometry.TransformBuffer, &transformBuffer));

				description.Triangles.Transform3x4 = transformBuffer->m_Resource->GetGPUVirtualAddress() + geometry.TransformOffset;
			}

			geometryDescriptions.push_back(description);
		}

		if (info.InstanceBuffer != nullptr)
		{
			Buffer* resolved = nullptr;
			SPALL_TRY(resolveInput(info.InstanceBuffer, &resolved));

			instanceBuffer = Resource<Buffer>(resolved);
		}

		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = accelerationStructureInputs(
			info.Type,
			info.Flags,
			geometryDescriptions,
			info.InstanceCount,
			0);

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_RayTracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

		if ((prebuildInfo.ResultDataMaxSizeInBytes == 0) or (prebuildInfo.ScratchDataSizeInBytes == 0))
		{
			return ERR_BACKEND_FAILURE;
		}

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_DEFAULT);

		ComPtr<ID3D12Resource> resource;
		const HRESULT hr = m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&resource));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		AccelerationStructureInfo structureInfo = {};
		structureInfo.Type = info.Type;
		structureInfo.Flags = info.Flags;
		structureInfo.Size = prebuildInfo.ResultDataMaxSizeInBytes;
		structureInfo.BuildScratchSize = prebuildInfo.ScratchDataSizeInBytes;
		structureInfo.UpdateScratchSize = prebuildInfo.UpdateScratchDataSizeInBytes;
		structureInfo.GeometryCount = static_cast<std::uint32_t>(geometryDescriptions.size());
		structureInfo.InstanceCount = info.InstanceCount;
		structureInfo.DebugName = info.DebugName;

		ComPtr<ID3D12Resource> compactedSizeBuffer;
		ComPtr<ID3D12Resource> compactedSizeReadback;

		if (hasAnyFlag(info.Flags, AccelerationStructureBuildFlags::AllowCompaction))
		{
			D3D12_RESOURCE_DESC sizeDesc = resourceDesc;
			sizeDesc.Width = sizeof(std::uint64_t);
			sizeDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			const HRESULT sizeResult = m_Device->CreateCommittedResource(
				&properties,
				D3D12_HEAP_FLAG_NONE,
				&sizeDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&compactedSizeBuffer));

			if (FAILED(sizeResult))
			{
				return mapStatus(sizeResult);
			}

			D3D12_RESOURCE_DESC readbackDesc = sizeDesc;
			readbackDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			const D3D12_HEAP_PROPERTIES readbackProperties = heapProperties(D3D12_HEAP_TYPE_READBACK);

			const HRESULT readbackResult = m_Device->CreateCommittedResource(
				&readbackProperties,
				D3D12_HEAP_FLAG_NONE,
				&readbackDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&compactedSizeReadback));

			if (FAILED(readbackResult))
			{
				return mapStatus(readbackResult);
			}
		}

		AccelerationStructure* created = new AccelerationStructure(
			*this,
			structureInfo,
			std::move(resource),
			std::move(inputBuffers),
			std::move(geometryDescriptions),
			std::move(instanceBuffer),
			info.InstanceBufferOffset);

		created->m_CompactedSizeBuffer = std::move(compactedSizeBuffer);
		created->m_CompactedSizeReadback = std::move(compactedSizeReadback);

		*accelerationStructure = Resource<IAccelerationStructure>(created);

		return {};
	}
} // namespace spall::d3d12
