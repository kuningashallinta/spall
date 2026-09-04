// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <spall/Common/Alignment.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Common/D3D12_ExportName.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_HeapMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_PipelineMappings.h>
#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSet.h>
#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSetLayout.h>
#include <src/Backends/D3D12/Pipeline/ComputePipeline/D3D12_ComputePipeline.h>
#include <src/Backends/D3D12/Pipeline/GraphicsPipeline/D3D12_GraphicsPipeline.h>
#include <src/Backends/D3D12/Pipeline/RayTracingPipeline/D3D12_RayTracingPipeline.h>
#include <src/Backends/D3D12/Pipeline/RootSignature/D3D12_RootSignature.h>
#include <src/Backends/D3D12/Pipeline/Shader/D3D12_Shader.h>
#include <src/Common/DXGI/DXGIError.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common.h>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace spall::d3d12
{
	Status Device::createShader(
		const ShaderCreateInfo& info,
		Resource<IShader>* shader)
	{
		if (shader == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateShaderCreateInfo(info));

		std::vector<std::byte> bytecode(info.Bytecode.begin(), info.Bytecode.end());

		*shader = Resource<IShader>(new Shader(*this, info.Stage, std::move(bytecode)));

		return {};
	}

	Status Device::createResourceSetLayout(
		const ResourceSetLayoutCreateInfo& info,
		Resource<IResourceSetLayout>* resourceSetLayout)
	{
		if (resourceSetLayout == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateResourceSetLayoutCreateInfo(info));

		std::vector<ResourceBindingInfo> bindings(info.Bindings.begin(), info.Bindings.end());

		std::sort(
			bindings.begin(),
			bindings.end(),
			[](const ResourceBindingInfo& left, const ResourceBindingInfo& right)
		{
			return left.Binding < right.Binding;
		});

		*resourceSetLayout = Resource<IResourceSetLayout>(new ResourceSetLayout(*this, std::move(bindings)));

		return {};
	}

	Status Device::createResourceSet(
		const ResourceSetCreateInfo& info,
		Resource<IResourceSet>* resourceSet)
	{
		if (resourceSet == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateResourceSetCreateInfo(info));

		ResourceSetLayout* layout = backendCast<ResourceSetLayout>(info.Layout);

		if ((layout == nullptr) or (layout->m_Device.get() != this))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		std::vector<std::uint32_t> descriptorIndices;
		descriptorIndices.reserve(layout->m_Bindings.size());

		for (std::size_t bindingIndex = 0; bindingIndex < layout->m_Bindings.size(); ++bindingIndex)
		{
			std::uint32_t descriptorIndex = InvalidDescriptorIndex;
			const Status error = m_ShaderResourceDescriptors.allocate(&descriptorIndex);

			if (error != SUCCESS)
			{
				for (const std::uint32_t allocatedIndex : descriptorIndices)
				{
					m_ShaderResourceDescriptors.release(allocatedIndex);
				}

				return error;
			}

			descriptorIndices.push_back(descriptorIndex);
		}

		std::unique_ptr<ResourceSet> createdResourceSet = std::make_unique<ResourceSet>(*this, *layout, std::move(descriptorIndices));

		if (not info.Writes.empty())
		{
			SPALL_TRY(createdResourceSet->writeResources(info.Writes));
		}

		*resourceSet = Resource<IResourceSet>(createdResourceSet.release());

		return {};
	}

	Status Device::createPipeline(
		const PipelineCreateInfo& info,
		Resource<IPipeline>* pipeline)
	{
		if (pipeline == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validatePipelineCreateInfo(info));

		Shader* vertexShader = backendCast<Shader>(info.VertexShader.Module);
		Shader* fragmentShader = backendCast<Shader>(info.FragmentShader.Module);
		Shader* geometryShader = backendCast<Shader>(info.GeometryShader.Module);
		Shader* hullShader = backendCast<Shader>(info.TessellationControlShader.Module);
		Shader* domainShader = backendCast<Shader>(info.TessellationEvaluationShader.Module);

		if ((vertexShader == nullptr) or (vertexShader->m_Device.get() != this) or
			((fragmentShader != nullptr) and (fragmentShader->m_Device.get() != this)) or
			((geometryShader != nullptr) and (geometryShader->m_Device.get() != this)) or
			((hullShader != nullptr) and (hullShader->m_Device.get() != this)) or
			((domainShader != nullptr) and (domainShader->m_Device.get() != this)))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((vertexShader->m_Stage != ShaderStage::Vertex) or
			((fragmentShader != nullptr) and (fragmentShader->m_Stage != ShaderStage::Fragment)) or
			((geometryShader != nullptr) and (geometryShader->m_Stage != ShaderStage::Geometry)) or
			((hullShader != nullptr) and (hullShader->m_Stage != ShaderStage::TessellationControl)) or
			((domainShader != nullptr) and (domainShader->m_Stage != ShaderStage::TessellationEvaluation)))
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
		inputElements.reserve(info.VertexAttributes.size());

		for (const VertexAttributeInfo& attribute : info.VertexAttributes)
		{
			const DXGI_FORMAT attributeFormat = format(attribute.Format);

			if (attributeFormat == DXGI_FORMAT_UNKNOWN)
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			bool bindingFound = false;

			for (const VertexBindingInfo& vertexBinding : info.VertexBindings)
			{
				if (vertexBinding.Binding == attribute.Binding)
				{
					bindingFound = true;
					break;
				}
			}

			if (not bindingFound)
			{
				return ERR_INVALID_BINDING;
			}

			D3D12_INPUT_ELEMENT_DESC element = {};
			element.SemanticName = "ATTRIBUTE";
			element.SemanticIndex = attribute.Location;
			element.Format = attributeFormat;
			element.InputSlot = attribute.Binding;
			element.AlignedByteOffset = attribute.Offset;
			element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;
			inputElements.push_back(element);
		}

		std::unique_ptr<RootSignature> rootSignature;
		SPALL_TRY(RootSignature::create(*this, info.ResourceSetLayouts, info.PushConstants, PipelineType::Graphics, &rootSignature));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
		pipelineDesc.pRootSignature = rootSignature->m_RootSignature.Get();
		pipelineDesc.VS.pShaderBytecode = vertexShader->m_Bytecode.data();
		pipelineDesc.VS.BytecodeLength = vertexShader->m_Bytecode.size();

		if (fragmentShader != nullptr)
		{
			pipelineDesc.PS.pShaderBytecode = fragmentShader->m_Bytecode.data();
			pipelineDesc.PS.BytecodeLength = fragmentShader->m_Bytecode.size();
		}

		if (geometryShader != nullptr)
		{
			pipelineDesc.GS.pShaderBytecode = geometryShader->m_Bytecode.data();
			pipelineDesc.GS.BytecodeLength = geometryShader->m_Bytecode.size();
		}

		if (hullShader != nullptr)
		{
			pipelineDesc.HS.pShaderBytecode = hullShader->m_Bytecode.data();
			pipelineDesc.HS.BytecodeLength = hullShader->m_Bytecode.size();
		}

		if (domainShader != nullptr)
		{
			pipelineDesc.DS.pShaderBytecode = domainShader->m_Bytecode.data();
			pipelineDesc.DS.BytecodeLength = domainShader->m_Bytecode.size();
		}

		pipelineDesc.BlendState.AlphaToCoverageEnable = FALSE;
		pipelineDesc.BlendState.IndependentBlendEnable = TRUE;

		for (std::uint32_t targetIndex = 0; targetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++targetIndex)
		{
			pipelineDesc.BlendState.RenderTarget[targetIndex] = (targetIndex < info.ColorTargetFormatCount)
				? blendState(info.BlendStates[targetIndex])
				: blendState(BlendStateInfo {});
		}

		pipelineDesc.SampleMask = UINT_MAX;

		pipelineDesc.RasterizerState.FillMode = fillMode(info.FillMode);
		pipelineDesc.RasterizerState.CullMode = cullMode(info.CullMode);
		pipelineDesc.RasterizerState.FrontCounterClockwise = frontCounterClockwise(info.FrontFace);
		pipelineDesc.RasterizerState.DepthBias = info.DepthBias;
		pipelineDesc.RasterizerState.DepthBiasClamp = info.DepthBiasClamp;
		pipelineDesc.RasterizerState.SlopeScaledDepthBias = info.SlopeScaledDepthBias;
		pipelineDesc.RasterizerState.DepthClipEnable = TRUE;
		pipelineDesc.RasterizerState.MultisampleEnable = (info.SampleCount > 1) ? TRUE : FALSE;
		pipelineDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		pipelineDesc.RasterizerState.ForcedSampleCount = 0;
		pipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		pipelineDesc.DepthStencilState.DepthEnable = info.EnableDepthTest ? TRUE : FALSE;
		pipelineDesc.DepthStencilState.DepthWriteMask = info.EnableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		pipelineDesc.DepthStencilState.DepthFunc = compareOp(info.DepthCompareOp);
		pipelineDesc.DepthStencilState.StencilEnable = info.EnableStencilTest ? TRUE : FALSE;
		pipelineDesc.DepthStencilState.StencilReadMask = info.StencilReadMask;
		pipelineDesc.DepthStencilState.StencilWriteMask = info.StencilWriteMask;
		pipelineDesc.DepthStencilState.FrontFace = stencilFaceDescription(info.FrontStencilState);
		pipelineDesc.DepthStencilState.BackFace = stencilFaceDescription(info.BackStencilState);

		pipelineDesc.InputLayout.pInputElementDescs = inputElements.empty() ? nullptr : inputElements.data();
		pipelineDesc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());
		pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		pipelineDesc.PrimitiveTopologyType = primitiveTopologyType(info.PrimitiveTopology);
		pipelineDesc.NumRenderTargets = info.ColorTargetFormatCount;

		for (std::uint32_t targetIndex = 0; targetIndex < info.ColorTargetFormatCount; ++targetIndex)
		{
			pipelineDesc.RTVFormats[targetIndex] = format(info.ColorTargetFormats[targetIndex]);
		}

		pipelineDesc.DSVFormat = format(info.DepthStencilFormat);
		pipelineDesc.SampleDesc.Count = info.SampleCount;
		pipelineDesc.SampleDesc.Quality = 0;
		pipelineDesc.NodeMask = 0;
		pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		ComPtr<ID3D12PipelineState> pipelineState;
		const HRESULT hr = m_Device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		std::vector<VertexBindingInfo> vertexBindings(info.VertexBindings.begin(), info.VertexBindings.end());

		*pipeline = Resource<IPipeline>(new GraphicsPipeline(
			*this,
			std::move(rootSignature),
			std::move(pipelineState),
			primitiveTopology(info.PrimitiveTopology, info.PatchControlPoints),
			std::move(vertexBindings),
			info.StencilReference));

		return {};
	}

	Status Device::createComputePipeline(
		const ComputePipelineCreateInfo& info,
		Resource<IPipeline>* pipeline)
	{
		if (pipeline == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateComputePipelineCreateInfo(info));

		Shader* computeShader = backendCast<Shader>(info.ComputeShader.Module);

		if ((computeShader == nullptr) or (computeShader->m_Device.get() != this))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (computeShader->m_Stage != ShaderStage::Compute)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		std::unique_ptr<RootSignature> rootSignature;
		SPALL_TRY(RootSignature::create(*this, info.ResourceSetLayouts, info.PushConstants, PipelineType::Compute, &rootSignature));

		D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc = {};
		pipelineDesc.pRootSignature = rootSignature->m_RootSignature.Get();
		pipelineDesc.CS.pShaderBytecode = computeShader->m_Bytecode.data();
		pipelineDesc.CS.BytecodeLength = computeShader->m_Bytecode.size();
		pipelineDesc.NodeMask = 0;
		pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		ComPtr<ID3D12PipelineState> pipelineState;
		const HRESULT hr = m_Device->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		*pipeline = Resource<IPipeline>(new ComputePipeline(*this, std::move(rootSignature), std::move(pipelineState)));

		return {};
	}

	Status Device::createRayTracingPipeline(
		const RayTracingPipelineCreateInfo& info,
		Resource<IPipeline>* pipeline)
	{
		if (pipeline == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (not m_RayTracingDevice)
		{
			return ERR_UNSUPPORTED;
		}

		SPALL_TRY(validateRayTracingPipelineCreateInfo(info));

		const std::size_t missCount = info.MissShaders.size();
		const std::size_t hitGroupCount = info.HitGroups.size();
		const std::size_t maxExports = 1 + missCount + (hitGroupCount * 3);

		struct ExportRecord
		{
			Shader* Module = nullptr;
			const char* Entry = nullptr;
		};

		std::vector<ExportRecord> exportRecords;
		std::vector<std::wstring> exportNames;
		std::vector<std::wstring> sourceNames;
		exportRecords.reserve(maxExports);
		exportNames.reserve(maxExports);
		sourceNames.reserve(maxExports);

		const auto resolveStage = [this, &exportRecords, &exportNames, &sourceNames](const PipelineShaderStageInfo& stage, ShaderStage expectedStage, std::size_t* exportIndex) -> Status
		{
			Shader* module = backendCast<Shader>(stage.Module);

			if ((module == nullptr) or (module->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			if (module->m_Stage != expectedStage)
			{
				return ERR_INVALID_SHADER_STAGE;
			}

			for (std::size_t existing = 0; existing < exportRecords.size(); ++existing)
			{
				if ((exportRecords[existing].Module == module) and
					(std::strcmp(exportRecords[existing].Entry, stage.Entry) == 0))
				{
					*exportIndex = existing;
					return {};
				}
			}

			*exportIndex = exportRecords.size();
			exportRecords.push_back(ExportRecord {module, stage.Entry});
			exportNames.push_back(generatedExportName(L"export", static_cast<std::uint32_t>(*exportIndex)));
			sourceNames.push_back(widenExportName(stage.Entry));

			return {};
		};

		std::size_t rayGenerationExport = 0;
		SPALL_TRY(resolveStage(info.RayGenerationShader, ShaderStage::RayGeneration, &rayGenerationExport));

		std::vector<std::size_t> missExports;
		missExports.reserve(missCount);

		for (const PipelineShaderStageInfo& missShader : info.MissShaders)
		{
			std::size_t missExport = 0;
			SPALL_TRY(resolveStage(missShader, ShaderStage::Miss, &missExport));
			missExports.push_back(missExport);
		}

		constexpr std::size_t UnusedExport = SIZE_MAX;

		struct HitGroupExports
		{
			std::size_t ClosestHit = UnusedExport;
			std::size_t AnyHit = UnusedExport;
			std::size_t Intersection = UnusedExport;
		};

		std::vector<HitGroupExports> hitGroupExports;
		std::vector<std::wstring> hitGroupNames;
		hitGroupExports.reserve(hitGroupCount);
		hitGroupNames.reserve(hitGroupCount);

		for (std::size_t groupIndex = 0; groupIndex < hitGroupCount; ++groupIndex)
		{
			const RayTracingHitGroup& hitGroup = info.HitGroups[groupIndex];
			HitGroupExports exports;

			if (hitGroup.ClosestHitShader.Module != nullptr)
			{
				SPALL_TRY(resolveStage(hitGroup.ClosestHitShader, ShaderStage::ClosestHit, &exports.ClosestHit));
			}

			if (hitGroup.AnyHitShader.Module != nullptr)
			{
				SPALL_TRY(resolveStage(hitGroup.AnyHitShader, ShaderStage::AnyHit, &exports.AnyHit));
			}

			if (hitGroup.IntersectionShader.Module != nullptr)
			{
				SPALL_TRY(resolveStage(hitGroup.IntersectionShader, ShaderStage::Intersection, &exports.Intersection));
			}

			hitGroupExports.push_back(exports);
			hitGroupNames.push_back(generatedExportName(L"group", static_cast<std::uint32_t>(groupIndex)));
		}

		std::vector<Shader*> modules;
		modules.reserve(exportRecords.size());

		for (const ExportRecord& record : exportRecords)
		{
			bool known = false;

			for (Shader* module : modules)
			{
				if (module == record.Module)
				{
					known = true;
					break;
				}
			}

			if (not known)
			{
				modules.push_back(record.Module);
			}
		}

		std::vector<std::vector<D3D12_EXPORT_DESC>> moduleExports(modules.size());
		std::vector<D3D12_DXIL_LIBRARY_DESC> libraries;
		std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescriptions;
		libraries.reserve(modules.size());
		hitGroupDescriptions.reserve(hitGroupCount);

		for (std::size_t moduleIndex = 0; moduleIndex < modules.size(); ++moduleIndex)
		{
			std::vector<D3D12_EXPORT_DESC>& exports = moduleExports[moduleIndex];
			exports.reserve(exportRecords.size());

			for (std::size_t exportIndex = 0; exportIndex < exportRecords.size(); ++exportIndex)
			{
				if (exportRecords[exportIndex].Module != modules[moduleIndex])
				{
					continue;
				}

				D3D12_EXPORT_DESC exportDescription = {};
				exportDescription.Name = exportNames[exportIndex].c_str();
				exportDescription.ExportToRename = sourceNames[exportIndex].c_str();
				exports.push_back(exportDescription);
			}

			D3D12_DXIL_LIBRARY_DESC library = {};
			library.DXILLibrary.pShaderBytecode = modules[moduleIndex]->m_Bytecode.data();
			library.DXILLibrary.BytecodeLength = modules[moduleIndex]->m_Bytecode.size();
			library.NumExports = static_cast<UINT>(exports.size());
			library.pExports = exports.data();
			libraries.push_back(library);
		}

		for (std::size_t groupIndex = 0; groupIndex < hitGroupCount; ++groupIndex)
		{
			const HitGroupExports& exports = hitGroupExports[groupIndex];

			D3D12_HIT_GROUP_DESC hitGroup = {};
			hitGroup.HitGroupExport = hitGroupNames[groupIndex].c_str();
			hitGroup.ClosestHitShaderImport = (exports.ClosestHit != UnusedExport) ? exportNames[exports.ClosestHit].c_str() : nullptr;
			hitGroup.AnyHitShaderImport = (exports.AnyHit != UnusedExport) ? exportNames[exports.AnyHit].c_str() : nullptr;
			hitGroup.IntersectionShaderImport = (exports.Intersection != UnusedExport) ? exportNames[exports.Intersection].c_str() : nullptr;
			hitGroup.Type = (exports.Intersection != UnusedExport)
				? D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE
				: D3D12_HIT_GROUP_TYPE_TRIANGLES;

			hitGroupDescriptions.push_back(hitGroup);
		}

		std::unique_ptr<RootSignature> rootSignature;
		SPALL_TRY(RootSignature::create(*this, info.ResourceSetLayouts, info.PushConstants, PipelineType::RayTracing, &rootSignature));

		D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
		shaderConfig.MaxPayloadSizeInBytes = info.MaxPayloadSize;
		shaderConfig.MaxAttributeSizeInBytes = info.MaxAttributeSize;

		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
		pipelineConfig.MaxTraceRecursionDepth = info.MaxRecursionDepth;

		D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {};
		globalRootSignature.pGlobalRootSignature = rootSignature->m_RootSignature.Get();

		std::vector<D3D12_STATE_SUBOBJECT> subobjects;
		subobjects.reserve(libraries.size() + hitGroupDescriptions.size() + 3);

		for (const D3D12_DXIL_LIBRARY_DESC& library : libraries)
		{
			subobjects.push_back(D3D12_STATE_SUBOBJECT {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &library});
		}

		for (const D3D12_HIT_GROUP_DESC& hitGroup : hitGroupDescriptions)
		{
			subobjects.push_back(D3D12_STATE_SUBOBJECT {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroup});
		}

		subobjects.push_back(D3D12_STATE_SUBOBJECT {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig});
		subobjects.push_back(D3D12_STATE_SUBOBJECT {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig});
		subobjects.push_back(D3D12_STATE_SUBOBJECT {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRootSignature});

		D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
		stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
		stateObjectDesc.pSubobjects = subobjects.data();

		ComPtr<ID3D12StateObject> stateObject;
		HRESULT hr = m_RayTracingDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		hr = stateObject.As(&stateObjectProperties);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		constexpr std::uint64_t identifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		constexpr std::uint64_t recordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
		constexpr std::uint64_t tableAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		const std::uint64_t missOffset = tableAlignment;
		std::uint64_t missSize = 0;
		std::uint64_t hitSize = 0;

		SPALL_TRY(Alignment::up(missCount * recordStride, tableAlignment, &missSize));
		SPALL_TRY(Alignment::up(hitGroupCount * recordStride, tableAlignment, &hitSize));

		const std::uint64_t hitOffset = missOffset + missSize;
		const std::uint64_t tableSize = hitOffset + hitSize;

		std::vector<std::byte> tableData(static_cast<std::size_t>(tableSize));

		const auto writeIdentifier = [&stateObjectProperties, &tableData](const std::wstring& name, std::uint64_t offset) -> Status
		{
			const void* identifier = stateObjectProperties->GetShaderIdentifier(name.c_str());

			if (identifier == nullptr)
			{
				return ERR_BACKEND_FAILURE;
			}

			std::memcpy(tableData.data() + offset, identifier, identifierSize);

			return {};
		};

		SPALL_TRY(writeIdentifier(exportNames[rayGenerationExport], 0));

		for (std::size_t missIndex = 0; missIndex < missCount; ++missIndex)
		{
			SPALL_TRY(writeIdentifier(exportNames[missExports[missIndex]], missOffset + (missIndex * recordStride)));
		}

		for (std::size_t groupIndex = 0; groupIndex < hitGroupCount; ++groupIndex)
		{
			SPALL_TRY(writeIdentifier(hitGroupNames[groupIndex], hitOffset + (groupIndex * recordStride)));
		}

		D3D12_RESOURCE_DESC tableDesc = {};
		tableDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		tableDesc.Width = tableSize;
		tableDesc.Height = 1;
		tableDesc.DepthOrArraySize = 1;
		tableDesc.MipLevels = 1;
		tableDesc.Format = DXGI_FORMAT_UNKNOWN;
		tableDesc.SampleDesc.Count = 1;
		tableDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_UPLOAD);

		ComPtr<ID3D12Resource> shaderTable;
		hr = m_Device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&tableDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&shaderTable));

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		void* mapped = nullptr;
		const D3D12_RANGE readRange = {0, 0};
		hr = shaderTable->Map(0, &readRange, &mapped);

		if (FAILED(hr))
		{
			return mapStatus(hr);
		}

		std::memcpy(mapped, tableData.data(), tableData.size());
		shaderTable->Unmap(0, nullptr);

		const D3D12_GPU_VIRTUAL_ADDRESS tableAddress = shaderTable->GetGPUVirtualAddress();

		D3D12_DISPATCH_RAYS_DESC dispatchDescription = {};
		dispatchDescription.RayGenerationShaderRecord.StartAddress = tableAddress;
		dispatchDescription.RayGenerationShaderRecord.SizeInBytes = recordStride;

		if (missCount != 0)
		{
			dispatchDescription.MissShaderTable.StartAddress = tableAddress + missOffset;
			dispatchDescription.MissShaderTable.SizeInBytes = missCount * recordStride;
			dispatchDescription.MissShaderTable.StrideInBytes = recordStride;
		}

		if (hitGroupCount != 0)
		{
			dispatchDescription.HitGroupTable.StartAddress = tableAddress + hitOffset;
			dispatchDescription.HitGroupTable.SizeInBytes = hitGroupCount * recordStride;
			dispatchDescription.HitGroupTable.StrideInBytes = recordStride;
		}

		*pipeline = Resource<IPipeline>(new RayTracingPipeline(
			*this,
			std::move(rootSignature),
			std::move(stateObject),
			std::move(shaderTable),
			dispatchDescription));

		return {};
	}
} // namespace spall::d3d12
