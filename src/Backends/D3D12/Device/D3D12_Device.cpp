// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <src/Backends/D3D12/CommandList/D3D12_CommandList.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_FormatMappings.h>
#include <src/Backends/D3D12/Queue/D3D12_ComputeQueue.h>
#include <src/Backends/D3D12/Queue/D3D12_GraphicsQueue.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace spall::d3d12
{
	Device::Device(
		ComPtr<IDXGIFactory4> factory,
		ComPtr<ID3D12Device> device)
		: m_Factory(std::move(factory)), m_Device(std::move(device))
	{
	}

	Device::~Device()
	{
		if (m_GraphicsQueue)
		{
			m_GraphicsQueue->waitIdle();
		}

		if (m_ComputeQueue)
		{
			m_ComputeQueue->waitIdle();
		}
	}

	Status Device::initialize()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		const HRESULT hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		m_GraphicsQueue = std::make_unique<GraphicsQueue>(*this, *m_CommandQueue.Get());

		SPALL_TRY(m_GraphicsQueue->initialize());

		D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
		computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		computeQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		const HRESULT computeHr = m_Device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_ComputeCommandQueue));

		if (FAILED(computeHr))
		{
			return mapStatus(computeHr);
		}

		m_ComputeQueue = std::make_unique<ComputeQueue>(*this, *m_ComputeCommandQueue.Get());

		SPALL_TRY(m_ComputeQueue->initialize());
		SPALL_TRY(m_RenderTargetViews.initialize(*m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RenderTargetViewHeapCapacity));
		SPALL_TRY(m_DepthStencilViews.initialize(*m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DepthStencilViewHeapCapacity));
		SPALL_TRY(m_ShaderResourceDescriptors.initialize(*m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ShaderResourceHeapCapacity));
		SPALL_TRY(m_SamplerDescriptors.initialize(*m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SamplerHeapCapacity));
		SPALL_TRY(createIndirectCommandSignatures());

		m_Limits.MaxTexture2DDimension = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		m_Limits.MaxFramebufferWidth = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		m_Limits.MaxFramebufferHeight = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		m_Limits.MaxUniformBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
		m_Limits.MaxVertexBuffers = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
		m_Limits.MaxVertexAttributes = D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT;
		m_Limits.MaxVertexBufferStride = D3D12_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES;
		m_Limits.MaxUniformBuffersPerStage = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
		m_Limits.MaxSampledTexturesPerStage = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
		m_Limits.MaxComputeStorageBuffers = D3D12_PS_CS_UAV_REGISTER_COUNT;
		m_Limits.MaxComputeStorageTextures = D3D12_PS_CS_UAV_REGISTER_COUNT;
		m_Limits.MaxColorAttachments = (std::min)(static_cast<std::uint32_t>(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT), MaxColorAttachments);
		m_Limits.MaxResourceSets = MaxResourceSets;
		m_Limits.MaxPushConstantSize = MaxPushConstantSize;
		m_Limits.MaxComputeWorkGroupCount[0] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
		m_Limits.MaxComputeWorkGroupCount[1] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
		m_Limits.MaxComputeWorkGroupCount[2] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
		m_Limits.MaxComputeWorkGroupSize[0] = D3D12_CS_THREAD_GROUP_MAX_X;
		m_Limits.MaxComputeWorkGroupSize[1] = D3D12_CS_THREAD_GROUP_MAX_Y;
		m_Limits.MaxComputeWorkGroupSize[2] = D3D12_CS_THREAD_GROUP_MAX_Z;
		m_Limits.MaxComputeWorkGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
		m_Limits.MaxSamplerAnisotropy = static_cast<float>(D3D12_MAX_MAXANISOTROPY);

		if (SUCCEEDED(m_CommandQueue->GetTimestampFrequency(&m_TimestampFrequency)) and (m_TimestampFrequency != 0))
		{
			m_Limits.SupportsTimestampQueries = true;
		}

		if (SUCCEEDED(m_Device.As(&m_RayTracingDevice)))
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};

			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options))) and
				(options.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1))
			{
				m_Limits.SupportsInlineRayTracing = true;
				m_Limits.SupportsRayTracingPipeline = true;
				m_Limits.MaxRayRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
			}
			else
			{
				m_RayTracingDevice.Reset();
			}
		}

		m_Limits.SupportedSampleCounts = 1;

		for (std::uint32_t sampleCount = 2; sampleCount <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT; sampleCount *= 2)
		{
			D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {};
			qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			qualityLevels.SampleCount = sampleCount;

			if (SUCCEEDED(m_Device->CheckFeatureSupport(
					D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
					&qualityLevels,
					sizeof(qualityLevels))) and
				(qualityLevels.NumQualityLevels != 0))
			{
				m_Limits.SupportedSampleCounts |= sampleCount;
			}
		}

		return {};
	}

	Status Device::createIndirectCommandSignatures()
	{
		const D3D12_INDIRECT_ARGUMENT_TYPE argumentTypes[] = {
			D3D12_INDIRECT_ARGUMENT_TYPE_DRAW,
			D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
			D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH};

		const UINT argumentStrides[] = {
			sizeof(D3D12_DRAW_ARGUMENTS),
			sizeof(D3D12_DRAW_INDEXED_ARGUMENTS),
			sizeof(D3D12_DISPATCH_ARGUMENTS)};

		ComPtr<ID3D12CommandSignature>* signatures[] = {
			&m_DrawSignature,
			&m_DrawIndexedSignature,
			&m_DispatchSignature};

		for (std::uint32_t signatureIndex = 0; signatureIndex < 3; ++signatureIndex)
		{
			D3D12_INDIRECT_ARGUMENT_DESC argumentDesc = {};
			argumentDesc.Type = argumentTypes[signatureIndex];

			D3D12_COMMAND_SIGNATURE_DESC signatureDesc = {};
			signatureDesc.ByteStride = argumentStrides[signatureIndex];
			signatureDesc.NumArgumentDescs = 1;
			signatureDesc.pArgumentDescs = &argumentDesc;
			signatureDesc.NodeMask = 0;

			ComPtr<ID3D12CommandSignature> signature;
			const HRESULT hr = m_Device->CreateCommandSignature(&signatureDesc, nullptr, IID_PPV_ARGS(&signature));

			if (FAILED(hr))
			{
				return mapStatus(hr);
			}

			*signatures[signatureIndex] = std::move(signature);
		}

		return {};
	}

	RenderBackendType Device::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	const DeviceLimits& Device::limits() const
	{
		return m_Limits;
	}

	Status Device::queryFormatCapabilities(
		Format format,
		FormatCapabilities* capabilities) const
	{
		SPALL_TRY(validateFormatCapabilityQuery(format, capabilities));

		D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};
		formatSupport.Format = d3d12::format(format);

		if (formatSupport.Format == DXGI_FORMAT_UNKNOWN)
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		const HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		*capabilities = formatCapabilities(format, formatSupport.Support1);

		return {};
	}

	IGraphicsQueue& Device::graphicsQueue()
	{
		return *m_GraphicsQueue;
	}

	IQueue& Device::computeQueue()
	{
		return *m_ComputeQueue;
	}

	Status Device::createCommandList(
		QueueType type,
		Resource<ICommandList>* commandList)
	{
		if (commandList == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		std::unique_ptr<CommandList> createdCommandList = std::make_unique<CommandList>(*this, type);

		SPALL_TRY(createdCommandList->initialize());

		*commandList = Resource<ICommandList>(createdCommandList.release());

		return {};
	}

	IResourceFactory& Device::resources()
	{
		return *this;
	}

	IPipelineFactory& Device::pipelines()
	{
		return *this;
	}

	IPresentationFactory& Device::presentation()
	{
		return *this;
	}
} // namespace spall::d3d12
