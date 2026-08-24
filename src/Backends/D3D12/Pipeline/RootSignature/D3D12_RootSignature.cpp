// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Pipeline/RootSignature/D3D12_RootSignature.h>

#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_PipelineMappings.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSetLayout.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <utility>

namespace spall::d3d12
{
	RootSignature::~RootSignature() = default;

	std::uint32_t RootSignature::setCount() const
	{
		return static_cast<std::uint32_t>(m_SetTables.size());
	}

	const RootSignature::SetTables& RootSignature::setTables(
		std::uint32_t slot) const
	{
		SPALL_ASSERT(slot < m_SetTables.size());

		return m_SetTables[slot];
	}

	const ResourceSetLayout* RootSignature::layout(
		std::uint32_t slot) const
	{
		SPALL_ASSERT(slot < m_Layouts.size());

		return m_Layouts[slot].get();
	}

	Status RootSignature::create(
		Device& device,
		std::span<const IResourceSetLayout* const> layouts,
		const PushConstantInfo& pushConstants,
		PipelineType type,
		std::unique_ptr<RootSignature>* rootSignature)
	{
		SPALL_ASSERT(rootSignature != nullptr);

		std::unique_ptr<RootSignature> created = std::unique_ptr<RootSignature>(new RootSignature());

		std::vector<D3D12_ROOT_PARAMETER> parameters;
		std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> viewRanges;
		std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> samplerRanges;

		viewRanges.reserve(layouts.size());
		samplerRanges.reserve(layouts.size());

		std::uint32_t nextRegisterOffset = 0;

		for (const IResourceSetLayout* layout : layouts)
		{
			ResourceSetLayout* backendLayout = backendCast<ResourceSetLayout>(const_cast<IResourceSetLayout*>(layout));

			if ((backendLayout == nullptr) or (backendLayout->m_Device.get() != &device))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			SetTables tables = {};
			tables.RegisterOffset = nextRegisterOffset;

			std::vector<D3D12_DESCRIPTOR_RANGE> views;
			std::vector<D3D12_DESCRIPTOR_RANGE> samplers;
			ShaderStageFlags viewStages = ShaderStageFlags::None;
			ShaderStageFlags samplerStages = ShaderStageFlags::None;

			for (const ResourceBindingInfo& bindingInfo : backendLayout->m_Bindings)
			{
				const std::uint32_t shaderRegister = nextRegisterOffset + bindingInfo.Binding;

				if ((bindingInfo.Type == ResourceBindingType::UniformBuffer) and
					(shaderRegister == PushConstantRegister) and
					((bindingInfo.Stages & pushConstants.Stages) != ShaderStageFlags::None))
				{
					return ERR_INVALID_BINDING;
				}

				D3D12_DESCRIPTOR_RANGE range = {};
				range.NumDescriptors = 1;
				range.BaseShaderRegister = shaderRegister;
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = static_cast<UINT>(views.size());

				switch (bindingInfo.Type)
				{
					case ResourceBindingType::UniformBuffer:
					{
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					}

					case ResourceBindingType::SampledTexture:
					case ResourceBindingType::AccelerationStructure:
					{
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					}

					case ResourceBindingType::StorageBuffer:
					case ResourceBindingType::StorageTexture:
					default:
					{
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					}
				}

				views.push_back(range);
				viewStages |= bindingInfo.Stages;

				if (bindingInfo.Type == ResourceBindingType::SampledTexture)
				{
					D3D12_DESCRIPTOR_RANGE samplerRange = {};
					samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					samplerRange.NumDescriptors = 1;
					samplerRange.BaseShaderRegister = shaderRegister;
					samplerRange.RegisterSpace = 0;
					samplerRange.OffsetInDescriptorsFromTableStart = static_cast<UINT>(samplers.size());

					samplers.push_back(samplerRange);
					samplerStages |= bindingInfo.Stages;
				}
			}

			nextRegisterOffset += backendLayout->registerSpan();

			tables.ViewDescriptorCount = static_cast<std::uint32_t>(views.size());
			tables.SamplerDescriptorCount = static_cast<std::uint32_t>(samplers.size());

			if (not views.empty())
			{
				tables.ViewParameterIndex = static_cast<std::uint32_t>(parameters.size());

				D3D12_ROOT_PARAMETER parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				parameter.ShaderVisibility = shaderStageFlags(viewStages);
				parameter.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(views.size());
				parameters.push_back(parameter);
			}

			if (not samplers.empty())
			{
				tables.SamplerParameterIndex = static_cast<std::uint32_t>(parameters.size());

				D3D12_ROOT_PARAMETER parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				parameter.ShaderVisibility = shaderStageFlags(samplerStages);
				parameter.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(samplers.size());
				parameters.push_back(parameter);
			}

			viewRanges.push_back(std::move(views));
			samplerRanges.push_back(std::move(samplers));

			created->m_Layouts.push_back(Resource<ResourceSetLayout>(backendLayout));
			created->m_SetTables.push_back(tables);
		}

		for (std::uint32_t slot = 0; slot < created->m_SetTables.size(); ++slot)
		{
			const SetTables& tables = created->m_SetTables[slot];

			if (tables.ViewParameterIndex != InvalidRootParameter)
			{
				parameters[tables.ViewParameterIndex].DescriptorTable.pDescriptorRanges = viewRanges[slot].data();
			}

			if (tables.SamplerParameterIndex != InvalidRootParameter)
			{
				parameters[tables.SamplerParameterIndex].DescriptorTable.pDescriptorRanges = samplerRanges[slot].data();
			}
		}

		if (pushConstants.Size != 0)
		{
			created->m_PushConstantParameterIndex = static_cast<std::uint32_t>(parameters.size());
			created->m_PushConstantSize = pushConstants.Size;
			created->m_PushConstantStages = pushConstants.Stages;

			D3D12_ROOT_PARAMETER parameter = {};
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			parameter.ShaderVisibility = shaderStageFlags(pushConstants.Stages);
			parameter.Constants.ShaderRegister = PushConstantRegister;
			parameter.Constants.RegisterSpace = 0;
			parameter.Constants.Num32BitValues = pushConstants.Size / 4;
			parameters.push_back(parameter);
		}

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = static_cast<UINT>(parameters.size());
		rootSignatureDesc.pParameters = parameters.empty() ? nullptr : parameters.data();
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.pStaticSamplers = nullptr;
		rootSignatureDesc.Flags = (type == PipelineType::Graphics)
			? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			: D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ComPtr<ID3DBlob> serialized;
		ComPtr<ID3DBlob> errors;
		HRESULT hr = D3D12SerializeRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serialized,
			&errors);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		hr = device.m_Device->CreateRootSignature(
			0,
			serialized->GetBufferPointer(),
			serialized->GetBufferSize(),
			IID_PPV_ARGS(&created->m_RootSignature));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		*rootSignature = std::move(created);

		return {};
	}
} // namespace spall::d3d12
