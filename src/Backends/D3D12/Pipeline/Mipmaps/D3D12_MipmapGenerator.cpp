// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Pipeline/Mipmaps/D3D12_MipmapGenerator.h>

#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Pipeline/Mipmaps/D3D12_DownsampleShaders.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <climits>
#include <mutex>
#include <utility>

namespace spall::d3d12
{
	Status MipmapGenerator::createRootSignature(
		Device& device)
	{
		D3D12_DESCRIPTOR_RANGE sourceRange = {};
		sourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		sourceRange.NumDescriptors = 1;
		sourceRange.BaseShaderRegister = 0;
		sourceRange.RegisterSpace = 0;
		sourceRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER parameter = {};
		parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameter.DescriptorTable.NumDescriptorRanges = 1;
		parameter.DescriptorTable.pDescriptorRanges = &sourceRange;

		D3D12_STATIC_SAMPLER_DESC staticSampler = {};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.ShaderRegister = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = 1;
		rootSignatureDesc.pParameters = &parameter;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &staticSampler;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		ComPtr<ID3DBlob> serialized;
		ComPtr<ID3DBlob> errors;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		hr = device.m_Device->CreateRootSignature(
			0,
			serialized->GetBufferPointer(),
			serialized->GetBufferSize(),
			IID_PPV_ARGS(&m_RootSignature));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		return {};
	}

	Status MipmapGenerator::createVolumeRootSignature(
		Device& device)
	{
		D3D12_DESCRIPTOR_RANGE sourceRange = {};
		sourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		sourceRange.NumDescriptors = 1;
		sourceRange.BaseShaderRegister = 0;
		sourceRange.RegisterSpace = 0;
		sourceRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER parameters[2] = {};
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameters[0].DescriptorTable.NumDescriptorRanges = 1;
		parameters[0].DescriptorTable.pDescriptorRanges = &sourceRange;
		parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameters[1].Constants.ShaderRegister = 0;
		parameters[1].Constants.RegisterSpace = 0;
		parameters[1].Constants.Num32BitValues = 1;

		D3D12_STATIC_SAMPLER_DESC staticSampler = {};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.ShaderRegister = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = 2;
		rootSignatureDesc.pParameters = parameters;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &staticSampler;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		ComPtr<ID3DBlob> serialized;
		ComPtr<ID3DBlob> errors;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		hr = device.m_Device->CreateRootSignature(
			0,
			serialized->GetBufferPointer(),
			serialized->GetBufferSize(),
			IID_PPV_ARGS(&m_VolumeRootSignature));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		return {};
	}

	Status MipmapGenerator::pipelineState(
		Device& device,
		TextureType type,
		Format format,
		ID3D12RootSignature** rootSignature,
		ID3D12PipelineState** pipelineState)
	{
		const std::lock_guard<std::mutex> guard(m_Mutex);

		const bool volume = (type == TextureType::Texture3D);

		if (volume)
		{
			if (not m_VolumeRootSignature)
			{
				SPALL_TRY(createVolumeRootSignature(device));
			}
		}
		else if (not m_RootSignature)
		{
			SPALL_TRY(createRootSignature(device));
		}

		ID3D12RootSignature* const signature = volume ? m_VolumeRootSignature.Get() : m_RootSignature.Get();

		const DXGI_FORMAT nativeTargetFormat = d3d12::format(format);
		const std::uint64_t key = (static_cast<std::uint64_t>(type) << 32) | static_cast<std::uint64_t>(nativeTargetFormat);
		const auto cached = m_PipelineStates.find(key);

		if (cached != m_PipelineStates.end())
		{
			*rootSignature = signature;
			*pipelineState = cached->second.Get();

			return {};
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
		pipelineDesc.pRootSignature = signature;
		pipelineDesc.VS.pShaderBytecode = DownsampleVertexBytecode;
		pipelineDesc.VS.BytecodeLength = sizeof(DownsampleVertexBytecode);

		switch (type)
		{
			case TextureType::Texture1D:
			{
				pipelineDesc.PS.pShaderBytecode = Downsample1DFragmentBytecode;
				pipelineDesc.PS.BytecodeLength = sizeof(Downsample1DFragmentBytecode);
				break;
			}

			case TextureType::Texture2D:
			{
				pipelineDesc.PS.pShaderBytecode = Downsample2DFragmentBytecode;
				pipelineDesc.PS.BytecodeLength = sizeof(Downsample2DFragmentBytecode);
				break;
			}

			case TextureType::Texture3D:
			{
				pipelineDesc.PS.pShaderBytecode = Downsample3DFragmentBytecode;
				pipelineDesc.PS.BytecodeLength = sizeof(Downsample3DFragmentBytecode);
				break;
			}
		}
		pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pipelineDesc.SampleMask = UINT_MAX;
		pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pipelineDesc.RasterizerState.DepthClipEnable = TRUE;
		pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RTVFormats[0] = nativeTargetFormat;
		pipelineDesc.SampleDesc.Count = 1;

		ComPtr<ID3D12PipelineState> createdPipelineState;
		const HRESULT hr = device.m_Device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&createdPipelineState));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		*rootSignature = signature;
		*pipelineState = createdPipelineState.Get();

		m_PipelineStates.emplace(key, std::move(createdPipelineState));

		return {};
	}
} // namespace spall::d3d12
