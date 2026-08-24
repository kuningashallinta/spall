// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <spall/Common/Alignment.h>
#include <src/Backends/Vulkan/Common/VK_EnumMappings.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSet.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>
#include <src/Backends/Vulkan/Pipeline/ComputePipeline/VK_ComputePipeline.h>
#include <src/Backends/Vulkan/Pipeline/GraphicsPipeline/VK_GraphicsPipeline.h>
#include <src/Backends/Vulkan/Pipeline/RayTracingPipeline/VK_RayTracingPipeline.h>
#include <src/Backends/Vulkan/Pipeline/Shader/VK_Shader.h>
#include <src/Validation/Backends/Vulkan/PipelineValidation.h>
#include <src/Validation/Common.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace spall::vk
{
	Status Device::createPipelineLayout(
		std::span<const IResourceSetLayout* const> layouts,
		const PushConstantInfo& pushConstants,
		std::vector<Resource<ResourceSetLayout>>* resourceSetLayouts,
		VkPipelineLayout* pipelineLayout)
	{
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		std::vector<ResourceBindingInfo> pipelineBindings;
		resourceSetLayouts->reserve(layouts.size());
		descriptorSetLayouts.reserve(layouts.size());

		for (const IResourceSetLayout* layout : layouts)
		{
			ResourceSetLayout* resourceSetLayout = dynamic_cast<ResourceSetLayout*>(const_cast<IResourceSetLayout*>(layout));

			if ((resourceSetLayout == nullptr) or (resourceSetLayout->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			resourceSetLayouts->emplace_back(resourceSetLayout);
			descriptorSetLayouts.push_back(resourceSetLayout->m_DescriptorSetLayout);
			pipelineBindings.insert(
				pipelineBindings.end(),
				resourceSetLayout->m_Bindings.begin(),
				resourceSetLayout->m_Bindings.end());
		}

		if (layouts.size() > m_Properties.limits.maxBoundDescriptorSets)
		{
			return ERR_INVALID_BINDING;
		}

		SPALL_TRY(PipelineValidation::validateDescriptorBindings(pipelineBindings, m_Properties.limits));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<std::uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.empty() ? nullptr : descriptorSetLayouts.data();
		VkPushConstantRange pushConstantRange = {};

		if (pushConstants.Size != 0)
		{
			if (pushConstants.Size > m_Properties.limits.maxPushConstantsSize)
			{
				return ERR_INVALID_SIZE;
			}

			pushConstantRange.stageFlags = shaderStageFlags(pushConstants.Stages);
			pushConstantRange.offset = 0;
			pushConstantRange.size = pushConstants.Size;
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		}

		const VkResult vkResult = vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, pipelineLayout);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		return {};
	}

	Status Device::createShader(
		const ShaderCreateInfo& info,
		Resource<IShader>* shader)
	{
		if (shader == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateShaderCreateInfo(info));

		if ((info.Bytecode.size() % sizeof(std::uint32_t)) != 0)
		{
			return ERR_INVALID_SHADER_BYTECODE;
		}

		std::vector<std::uint32_t> bytecode(info.Bytecode.size() / sizeof(std::uint32_t));
		std::memcpy(bytecode.data(), info.Bytecode.data(), info.Bytecode.size());

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = info.Bytecode.size();
		createInfo.pCode = bytecode.data();

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		const VkResult vkResult = vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		Shader* vkShader = new Shader(*this, info.Stage, std::move(bytecode), shaderModule);

		*shader = Resource<IShader>(vkShader);

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
		SPALL_TRY(PipelineValidation::validateDescriptorBindings(info.Bindings, m_Properties.limits));

		if (not m_RayTracingEnabled)
		{
			for (const ResourceBindingInfo& bindingInfo : info.Bindings)
			{
				if (bindingInfo.Type == ResourceBindingType::AccelerationStructure)
				{
					return ERR_UNSUPPORTED;
				}
			}
		}

		if (m_SupportedFeatures.fragmentStoresAndAtomics == VK_FALSE)
		{
			for (const ResourceBindingInfo& bindingInfo : info.Bindings)
			{
				const bool storage = (bindingInfo.Type == ResourceBindingType::StorageBuffer) or
					(bindingInfo.Type == ResourceBindingType::StorageTexture);

				if (storage and ((bindingInfo.Stages & ShaderStageFlags::Fragment) != ShaderStageFlags::None))
				{
					return ERR_UNSUPPORTED_USAGE;
				}
			}
		}

		std::vector<ResourceBindingInfo> bindings(info.Bindings.begin(), info.Bindings.end());
		std::vector<VkDescriptorSetLayoutBinding> descriptorBindings;
		descriptorBindings.reserve(bindings.size());

		for (const ResourceBindingInfo& bindingInfo : bindings)
		{
			VkDescriptorSetLayoutBinding descriptorBinding = {};
			descriptorBinding.binding = bindingInfo.Binding;
			descriptorBinding.descriptorType = descriptorType(bindingInfo.Type);
			descriptorBinding.descriptorCount = 1;
			descriptorBinding.stageFlags = shaderStageFlags(bindingInfo.Stages);
			descriptorBindings.push_back(descriptorBinding);
		}

		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = static_cast<std::uint32_t>(descriptorBindings.size());
		createInfo.pBindings = descriptorBindings.data();

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		const VkResult vkResult = vkCreateDescriptorSetLayout(m_Device, &createInfo, nullptr, &descriptorSetLayout);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		ResourceSetLayout* layout = new ResourceSetLayout(*this, std::move(bindings), descriptorSetLayout);

		*resourceSetLayout = Resource<IResourceSetLayout>(layout);

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

		ResourceSetLayout* layout = dynamic_cast<ResourceSetLayout*>(info.Layout);

		if ((layout == nullptr) or (layout->m_Device.get() != this))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		std::uint32_t uniformBufferCount = 0;
		std::uint32_t sampledTextureCount = 0;
		std::uint32_t storageBufferCount = 0;
		std::uint32_t storageTextureCount = 0;
		std::uint32_t accelerationStructureCount = 0;

		for (const ResourceBindingInfo& bindingInfo : layout->m_Bindings)
		{
			if (bindingInfo.Type == ResourceBindingType::UniformBuffer)
			{
				++uniformBufferCount;
			}
			else if (bindingInfo.Type == ResourceBindingType::SampledTexture)
			{
				++sampledTextureCount;
			}
			else if (bindingInfo.Type == ResourceBindingType::StorageBuffer)
			{
				++storageBufferCount;
			}
			else if (bindingInfo.Type == ResourceBindingType::StorageTexture)
			{
				++storageTextureCount;
			}
			else if (bindingInfo.Type == ResourceBindingType::AccelerationStructure)
			{
				++accelerationStructureCount;
			}
		}

		std::vector<VkDescriptorPoolSize> poolSizes;

		if (uniformBufferCount != 0)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize.descriptorCount = uniformBufferCount;
			poolSizes.push_back(poolSize);
		}

		if (sampledTextureCount != 0)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = sampledTextureCount;
			poolSizes.push_back(poolSize);
		}

		if (storageBufferCount != 0)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSize.descriptorCount = storageBufferCount;
			poolSizes.push_back(poolSize);
		}

		if (storageTextureCount != 0)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			poolSize.descriptorCount = storageTextureCount;
			poolSizes.push_back(poolSize);
		}

		if (accelerationStructureCount != 0)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			poolSize.descriptorCount = accelerationStructureCount;
			poolSizes.push_back(poolSize);
		}

		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.maxSets = 1;
		poolCreateInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
		poolCreateInfo.pPoolSizes = poolSizes.data();

		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		const VkResult poolResult = vkCreateDescriptorPool(m_Device, &poolCreateInfo, nullptr, &descriptorPool);

		if (poolResult != VK_SUCCESS)
		{
			return mapStatus(poolResult);
		}

		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &layout->m_DescriptorSetLayout;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		const VkResult vkResult = vkAllocateDescriptorSets(m_Device, &allocateInfo, &descriptorSet);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroyDescriptorPool(m_Device, descriptorPool, nullptr);
			return mapStatus(vkResult);
		}

		std::unique_ptr<ResourceSet> vkResourceSet = std::make_unique<ResourceSet>(*this, *layout, descriptorPool, descriptorSet);

		if (not info.Writes.empty())
		{
			SPALL_TRY(vkResourceSet->writeResources(info.Writes));
		}

		*resourceSet = Resource<IResourceSet>(vkResourceSet.release());

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
		SPALL_TRY(PipelineValidation::validateCreateInfo(info, m_SupportedFeatures, m_Properties.limits));

		Shader* vertexShader = dynamic_cast<Shader*>(info.VertexShader.Module);
		Shader* fragmentShader = info.FragmentShader.Module != nullptr ? dynamic_cast<Shader*>(info.FragmentShader.Module) : nullptr;
		Shader* geometryShader = info.GeometryShader.Module != nullptr ? dynamic_cast<Shader*>(info.GeometryShader.Module) : nullptr;
		Shader* tessellationControlShader = info.TessellationControlShader.Module != nullptr ? dynamic_cast<Shader*>(info.TessellationControlShader.Module) : nullptr;
		Shader* tessellationEvaluationShader = info.TessellationEvaluationShader.Module != nullptr
			? dynamic_cast<Shader*>(info.TessellationEvaluationShader.Module)
			: nullptr;

		if ((vertexShader == nullptr) or ((info.FragmentShader.Module != nullptr) and (fragmentShader == nullptr)) or
			((info.GeometryShader.Module != nullptr) and (geometryShader == nullptr)) or
			((info.TessellationControlShader.Module != nullptr) and (tessellationControlShader == nullptr)) or
			((info.TessellationEvaluationShader.Module != nullptr) and (tessellationEvaluationShader == nullptr)))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((vertexShader->m_Device.get() != this) or ((fragmentShader != nullptr) and (fragmentShader->m_Device.get() != this)) or
			((geometryShader != nullptr) and (geometryShader->m_Device.get() != this)) or
			((tessellationControlShader != nullptr) and (tessellationControlShader->m_Device.get() != this)) or
			((tessellationEvaluationShader != nullptr) and (tessellationEvaluationShader->m_Device.get() != this)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if ((vertexShader->m_Stage != ShaderStage::Vertex) or
			((fragmentShader != nullptr) and (fragmentShader->m_Stage != ShaderStage::Fragment)) or
			((geometryShader != nullptr) and (geometryShader->m_Stage != ShaderStage::Geometry)) or
			((tessellationControlShader != nullptr) and (tessellationControlShader->m_Stage != ShaderStage::TessellationControl)) or
			((tessellationEvaluationShader != nullptr) and (tessellationEvaluationShader->m_Stage != ShaderStage::TessellationEvaluation)))
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		std::vector<VkVertexInputBindingDescription> vertexBindings;
		vertexBindings.reserve(info.VertexBindings.size());
		std::vector<VertexBindingInfo> vertexBindingInfos(info.VertexBindings.begin(), info.VertexBindings.end());

		for (const VertexBindingInfo& binding : info.VertexBindings)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding.Binding;
			bindingDescription.stride = binding.Stride;
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindings.push_back(bindingDescription);
		}

		std::vector<VkVertexInputAttributeDescription> vertexAttributes;
		vertexAttributes.reserve(info.VertexAttributes.size());

		for (const VertexAttributeInfo& attribute : info.VertexAttributes)
		{
			const std::optional<VertexFormatProperties> formatInfo = vertexFormatInfo(attribute.Format);

			if (not formatInfo.has_value())
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			VkVertexInputAttributeDescription attributeDescription = {};
			attributeDescription.location = attribute.Location;
			attributeDescription.binding = attribute.Binding;
			attributeDescription.format = formatInfo->format;
			attributeDescription.offset = attribute.Offset;
			vertexAttributes.push_back(attributeDescription);
		}

		std::vector<Resource<ResourceSetLayout>> resourceSetLayouts;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		SPALL_TRY(createPipelineLayout(info.ResourceSetLayouts, info.PushConstants, &resourceSetLayouts, &pipelineLayout));

		VkResult vkResult = VK_SUCCESS;

		const VkRenderPass renderPass = this->renderPass(info.ColorTargetFormats, info.ColorTargetFormatCount, info.DepthStencilFormat, info.SampleCount);

		if (renderPass == VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_Device, pipelineLayout, nullptr);

			return ERR_RENDER_PASS_CREATION_FAILED;
		}

		VkPipelineShaderStageCreateInfo shaderStages[5] = {};
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertexShader->m_ShaderModule;
		shaderStages[0].pName = (info.VertexShader.Entry != nullptr) ? info.VertexShader.Entry : "main";

		std::uint32_t shaderStageCount = 1;

		if (fragmentShader != nullptr)
		{
			shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[shaderStageCount].module = fragmentShader->m_ShaderModule;
			shaderStages[shaderStageCount].pName = (info.FragmentShader.Entry != nullptr) ? info.FragmentShader.Entry : "main";
			++shaderStageCount;
		}

		if (geometryShader != nullptr)
		{
			shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			shaderStages[shaderStageCount].module = geometryShader->m_ShaderModule;
			shaderStages[shaderStageCount].pName = (info.GeometryShader.Entry != nullptr) ? info.GeometryShader.Entry : "main";
			++shaderStageCount;
		}

		if (tessellationControlShader != nullptr)
		{
			shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			shaderStages[shaderStageCount].module = tessellationControlShader->m_ShaderModule;
			shaderStages[shaderStageCount].pName = (info.TessellationControlShader.Entry != nullptr) ? info.TessellationControlShader.Entry : "main";
			++shaderStageCount;
		}

		if (tessellationEvaluationShader != nullptr)
		{
			shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			shaderStages[shaderStageCount].module = tessellationEvaluationShader->m_ShaderModule;
			shaderStages[shaderStageCount].pName = (info.TessellationEvaluationShader.Entry != nullptr) ? info.TessellationEvaluationShader.Entry : "main";
			++shaderStageCount;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertexBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexBindings.empty() ? nullptr : vertexBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexAttributes.empty() ? nullptr : vertexAttributes.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = primitiveTopology(info.PrimitiveTopology);
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineTessellationStateCreateInfo tessellationState = {};
		tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationState.patchControlPoints = info.PatchControlPoints;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = polygonMode(info.FillMode);
		rasterizationState.cullMode = cullMode(info.CullMode);
		rasterizationState.frontFace = frontFace(info.FrontFace);
		rasterizationState.depthBiasEnable = ((info.DepthBias != 0) or (info.DepthBiasClamp != 0.0f) or
												 (info.SlopeScaledDepthBias != 0.0f))
			? VK_TRUE
			: VK_FALSE;
		rasterizationState.depthBiasConstantFactor = static_cast<float>(info.DepthBias);
		rasterizationState.depthBiasClamp = info.DepthBiasClamp;
		rasterizationState.depthBiasSlopeFactor = info.SlopeScaledDepthBias;
		rasterizationState.lineWidth = info.LineWidth;

		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(info.SampleCount);

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = info.EnableDepthTest ? VK_TRUE : VK_FALSE;
		depthStencilState.depthWriteEnable = info.EnableDepthWrite ? VK_TRUE : VK_FALSE;
		depthStencilState.depthCompareOp = compareOp(info.DepthCompareOp);
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = info.EnableStencilTest ? VK_TRUE : VK_FALSE;
		depthStencilState.front.failOp = stencilOp(info.FrontStencilState.FailOp);
		depthStencilState.front.passOp = stencilOp(info.FrontStencilState.PassOp);
		depthStencilState.front.depthFailOp = stencilOp(info.FrontStencilState.DepthFailOp);
		depthStencilState.front.compareOp = compareOp(info.FrontStencilState.Compare);
		depthStencilState.front.compareMask = info.StencilReadMask;
		depthStencilState.front.writeMask = info.StencilWriteMask;
		depthStencilState.front.reference = info.StencilReference;
		depthStencilState.back.failOp = stencilOp(info.BackStencilState.FailOp);
		depthStencilState.back.passOp = stencilOp(info.BackStencilState.PassOp);
		depthStencilState.back.depthFailOp = stencilOp(info.BackStencilState.DepthFailOp);
		depthStencilState.back.compareOp = compareOp(info.BackStencilState.Compare);
		depthStencilState.back.compareMask = info.StencilReadMask;
		depthStencilState.back.writeMask = info.StencilWriteMask;
		depthStencilState.back.reference = info.StencilReference;

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
		colorBlendAttachments.reserve(info.ColorTargetFormatCount);

		for (std::uint32_t blendStateIndex = 0; blendStateIndex < info.ColorTargetFormatCount; ++blendStateIndex)
		{
			colorBlendAttachments.push_back(colorBlendAttachmentState(info.BlendStates[blendStateIndex]));
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.attachmentCount = static_cast<std::uint32_t>(colorBlendAttachments.size());
		colorBlendState.pAttachments = colorBlendAttachments.empty() ? nullptr : colorBlendAttachments.data();

		const VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 3;
		dynamicState.pDynamicStates = dynamicStates;

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.stageCount = shaderStageCount;
		graphicsPipelineCreateInfo.pStages = shaderStages;
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputState;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		graphicsPipelineCreateInfo.pTessellationState = (tessellationControlShader != nullptr) ? &tessellationState : nullptr;
		graphicsPipelineCreateInfo.pViewportState = &viewportState;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationState;
		graphicsPipelineCreateInfo.pMultisampleState = &multisampleState;
		graphicsPipelineCreateInfo.pDepthStencilState = (info.DepthStencilFormat != Format::Unknown) ? &depthStencilState : nullptr;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendState;
		graphicsPipelineCreateInfo.pDynamicState = &dynamicState;

		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = renderPass;
		graphicsPipelineCreateInfo.subpass = 0;

		VkPipeline vkPipeline = VK_NULL_HANDLE;
		vkResult = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &vkPipeline);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroyPipelineLayout(m_Device, pipelineLayout, nullptr);
			return mapStatus(vkResult);
		}

		GraphicsPipeline::Bindings bindings = {};
		bindings.VertexBindings = std::move(vertexBindingInfos);
		bindings.ResourceSetLayouts = std::move(resourceSetLayouts);

		GraphicsPipeline::Targets targets = {};
		targets.ColorFormats = info.ColorTargetFormats;
		targets.ColorFormatCount = info.ColorTargetFormatCount;
		targets.DepthStencilFormat = info.DepthStencilFormat;
		targets.SampleCount = info.SampleCount;
		targets.StencilReference = info.StencilReference;

		GraphicsPipeline::PushConstants pushConstants = {};
		pushConstants.Stages = info.PushConstants.Stages;
		pushConstants.Size = info.PushConstants.Size;

		GraphicsPipeline* graphicsPipeline = new GraphicsPipeline(
			*this,
			pipelineLayout,
			vkPipeline,
			std::move(bindings),
			targets,
			pushConstants);

		*pipeline = Resource<IPipeline>(graphicsPipeline);

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

		Shader* computeShader = dynamic_cast<Shader*>(info.ComputeShader.Module);

		if (computeShader == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (computeShader->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (computeShader->m_Stage != ShaderStage::Compute)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		std::vector<Resource<ResourceSetLayout>> resourceSetLayouts;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		SPALL_TRY(createPipelineLayout(info.ResourceSetLayouts, info.PushConstants, &resourceSetLayouts, &pipelineLayout));

		VkResult vkResult = VK_SUCCESS;

		VkComputePipelineCreateInfo computePipelineCreateInfo = {};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computePipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computePipelineCreateInfo.stage.module = computeShader->m_ShaderModule;
		computePipelineCreateInfo.stage.pName = (info.ComputeShader.Entry != nullptr) ? info.ComputeShader.Entry : "main";
		computePipelineCreateInfo.layout = pipelineLayout;

		VkPipeline vkPipeline = VK_NULL_HANDLE;
		vkResult = vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &vkPipeline);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroyPipelineLayout(m_Device, pipelineLayout, nullptr);
			return mapStatus(vkResult);
		}

		ComputePipeline* computePipeline = new ComputePipeline(
			*this,
			pipelineLayout,
			vkPipeline,
			std::move(resourceSetLayouts),
			info.PushConstants.Stages,
			info.PushConstants.Size);

		*pipeline = Resource<IPipeline>(computePipeline);

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

		if (not m_RayTracingPipelineEnabled)
		{
			return ERR_UNSUPPORTED;
		}

		SPALL_TRY(validateRayTracingPipelineCreateInfo(info));

		if (info.MaxRecursionDepth > m_MaxRayRecursionDepth)
		{
			return ERR_INVALID_RANGE;
		}

		const std::size_t missCount = info.MissShaders.size();
		const std::size_t hitGroupCount = info.HitGroups.size();

		struct StageRecord
		{
			Shader* Module = nullptr;
			const char* Entry = nullptr;
		};

		std::vector<StageRecord> stageRecords;
		std::vector<VkPipelineShaderStageCreateInfo> stages;
		stageRecords.reserve(1 + missCount + (hitGroupCount * 3));
		stages.reserve(1 + missCount + (hitGroupCount * 3));

		const auto resolveStage = [this, &stageRecords, &stages](const PipelineShaderStageInfo& stage, ShaderStage expectedStage, VkShaderStageFlagBits nativeStage, std::uint32_t* stageIndex) -> Status
		{
			Shader* module = dynamic_cast<Shader*>(stage.Module);

			if ((module == nullptr) or (module->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			if (module->m_Stage != expectedStage)
			{
				return ERR_INVALID_SHADER_STAGE;
			}

			for (std::size_t existing = 0; existing < stageRecords.size(); ++existing)
			{
				if ((stageRecords[existing].Module == module) and
					(std::strcmp(stageRecords[existing].Entry, stage.Entry) == 0))
				{
					*stageIndex = static_cast<std::uint32_t>(existing);
					return {};
				}
			}

			*stageIndex = static_cast<std::uint32_t>(stageRecords.size());
			stageRecords.push_back(StageRecord {module, stage.Entry});

			VkPipelineShaderStageCreateInfo stageCreateInfo = {};
			stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageCreateInfo.stage = nativeStage;
			stageCreateInfo.module = module->m_ShaderModule;
			stageCreateInfo.pName = stage.Entry;
			stages.push_back(stageCreateInfo);

			return {};
		};

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
		groups.reserve(1 + missCount + hitGroupCount);

		VkRayTracingShaderGroupCreateInfoKHR group = {};
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::uint32_t rayGenerationStage = 0;
		SPALL_TRY(resolveStage(info.RayGenerationShader, ShaderStage::RayGeneration, VK_SHADER_STAGE_RAYGEN_BIT_KHR, &rayGenerationStage));

		group.generalShader = rayGenerationStage;
		groups.push_back(group);

		for (const PipelineShaderStageInfo& missShader : info.MissShaders)
		{
			std::uint32_t missStage = 0;
			SPALL_TRY(resolveStage(missShader, ShaderStage::Miss, VK_SHADER_STAGE_MISS_BIT_KHR, &missStage));

			group.generalShader = missStage;
			groups.push_back(group);
		}

		for (const RayTracingHitGroup& hitGroup : info.HitGroups)
		{
			VkRayTracingShaderGroupCreateInfoKHR hitGroupCreateInfo = {};
			hitGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			hitGroupCreateInfo.type = (hitGroup.IntersectionShader.Module != nullptr)
				? VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR
				: VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			hitGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
			hitGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
			hitGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
			hitGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

			if (hitGroup.ClosestHitShader.Module != nullptr)
			{
				SPALL_TRY(resolveStage(hitGroup.ClosestHitShader, ShaderStage::ClosestHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, &hitGroupCreateInfo.closestHitShader));
			}

			if (hitGroup.AnyHitShader.Module != nullptr)
			{
				SPALL_TRY(resolveStage(hitGroup.AnyHitShader, ShaderStage::AnyHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, &hitGroupCreateInfo.anyHitShader));
			}

			if (hitGroup.IntersectionShader.Module != nullptr)
			{
				SPALL_TRY(resolveStage(hitGroup.IntersectionShader, ShaderStage::Intersection, VK_SHADER_STAGE_INTERSECTION_BIT_KHR, &hitGroupCreateInfo.intersectionShader));
			}

			groups.push_back(hitGroupCreateInfo);
		}

		std::vector<Resource<ResourceSetLayout>> resourceSetLayouts;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		SPALL_TRY(createPipelineLayout(info.ResourceSetLayouts, info.PushConstants, &resourceSetLayouts, &pipelineLayout));

		VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineCreateInfo.stageCount = static_cast<std::uint32_t>(stages.size());
		pipelineCreateInfo.pStages = stages.data();
		pipelineCreateInfo.groupCount = static_cast<std::uint32_t>(groups.size());
		pipelineCreateInfo.pGroups = groups.data();
		pipelineCreateInfo.maxPipelineRayRecursionDepth = info.MaxRecursionDepth;
		pipelineCreateInfo.layout = pipelineLayout;

		VkPipeline vkPipeline = VK_NULL_HANDLE;
		VkResult vkResult = m_CreateRayTracingPipelines(m_Device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vkPipeline);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroyPipelineLayout(m_Device, pipelineLayout, nullptr);
			return mapStatus(vkResult);
		}

		RayTracingPipeline* rayTracingPipeline = new RayTracingPipeline(
			*this,
			pipelineLayout,
			vkPipeline,
			std::move(resourceSetLayouts),
			info.PushConstants.Stages,
			info.PushConstants.Size);

		Resource<IPipeline> created(rayTracingPipeline);

		const VkDeviceSize handleSize = m_ShaderGroupHandleSize;
		const VkDeviceSize recordStride = Alignment::up(handleSize, m_ShaderGroupHandleAlignment);
		const VkDeviceSize rayGenerationSize = Alignment::up(recordStride, m_ShaderGroupBaseAlignment);
		const VkDeviceSize missOffset = rayGenerationSize;
		const VkDeviceSize hitOffset = missOffset + Alignment::up(missCount * recordStride, m_ShaderGroupBaseAlignment);
		const VkDeviceSize tableSize = hitOffset + Alignment::up(hitGroupCount * recordStride, m_ShaderGroupBaseAlignment);

		VkBufferCreateInfo tableCreateInfo = {};
		tableCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		tableCreateInfo.size = tableSize;
		tableCreateInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		tableCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		VkBuffer tableBuffer = VK_NULL_HANDLE;
		VmaAllocation tableAllocation = VK_NULL_HANDLE;
		vkResult = vmaCreateBufferWithAlignment(
			m_Allocator,
			&tableCreateInfo,
			&allocationCreateInfo,
			m_ShaderGroupBaseAlignment,
			&tableBuffer,
			&tableAllocation,
			nullptr);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		rayTracingPipeline->m_ShaderBindingTable = tableBuffer;
		rayTracingPipeline->m_ShaderBindingTableAllocation = tableAllocation;

		const std::uint32_t groupCount = static_cast<std::uint32_t>(groups.size());
		std::vector<std::uint8_t> handles(groupCount * static_cast<std::size_t>(handleSize));

		vkResult = m_GetRayTracingShaderGroupHandles(m_Device, vkPipeline, 0, groupCount, handles.size(), handles.data());

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		void* mapped = nullptr;
		vkResult = vmaMapMemory(m_Allocator, tableAllocation, &mapped);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		std::memset(mapped, 0, static_cast<std::size_t>(tableSize));

		const auto writeHandle = [&mapped, &handles, handleSize](std::size_t groupIndex, VkDeviceSize offset)
		{
			std::memcpy(
				static_cast<std::uint8_t*>(mapped) + offset,
				handles.data() + (groupIndex * static_cast<std::size_t>(handleSize)),
				static_cast<std::size_t>(handleSize));
		};

		writeHandle(0, 0);

		for (std::size_t missIndex = 0; missIndex < missCount; ++missIndex)
		{
			writeHandle(1 + missIndex, missOffset + (missIndex * recordStride));
		}

		for (std::size_t groupIndex = 0; groupIndex < hitGroupCount; ++groupIndex)
		{
			writeHandle(1 + missCount + groupIndex, hitOffset + (groupIndex * recordStride));
		}

		vmaFlushAllocation(m_Allocator, tableAllocation, 0, tableSize);
		vmaUnmapMemory(m_Allocator, tableAllocation);

		VkBufferDeviceAddressInfo addressInfo = {};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.buffer = tableBuffer;

		const VkDeviceAddress tableAddress = m_GetBufferDeviceAddress(m_Device, &addressInfo);

		rayTracingPipeline->m_RayGenerationRegion = VkStridedDeviceAddressRegionKHR {tableAddress, rayGenerationSize, rayGenerationSize};

		if (missCount != 0)
		{
			rayTracingPipeline->m_MissRegion = VkStridedDeviceAddressRegionKHR {tableAddress + missOffset, recordStride, missCount * recordStride};
		}

		if (hitGroupCount != 0)
		{
			rayTracingPipeline->m_HitRegion = VkStridedDeviceAddressRegionKHR {tableAddress + hitOffset, recordStride, hitGroupCount * recordStride};
		}

		*pipeline = std::move(created);

		return {};
	}

	VkRenderPass Device::renderPass(
		const Format* colorFormats,
		std::uint32_t colorFormatCount,
		Format depthFormat,
		std::uint32_t sampleCount)
	{
		for (const RenderPassCacheEntry& entry : m_RenderPassCache)
		{
			if ((entry.ColorFormatCount != colorFormatCount) or (entry.SampleCount != sampleCount))
			{
				continue;
			}

			bool colorFormatsMatch = true;

			for (std::uint32_t colorIndex = 0; colorIndex < colorFormatCount; ++colorIndex)
			{
				if (entry.ColorFormats[colorIndex] != colorFormats[colorIndex])
				{
					colorFormatsMatch = false;
					break;
				}
			}

			if (colorFormatsMatch and (entry.DepthFormat == depthFormat))
			{
				return entry.RenderPass;
			}
		}

		VkRenderPass renderPass = VK_NULL_HANDLE;
		const Status error = buildRenderPass(colorFormats, colorFormatCount, depthFormat, sampleCount, nullptr, nullptr, renderPass);

		if (error != SUCCESS)
		{
			return VK_NULL_HANDLE;
		}

		RenderPassCacheEntry cacheEntry = {};

		for (std::uint32_t colorIndex = 0; colorIndex < colorFormatCount; ++colorIndex)
		{
			cacheEntry.ColorFormats[colorIndex] = colorFormats[colorIndex];
		}

		cacheEntry.ColorFormatCount = colorFormatCount;
		cacheEntry.DepthFormat = depthFormat;
		cacheEntry.SampleCount = sampleCount;
		cacheEntry.RenderPass = renderPass;
		m_RenderPassCache.push_back(cacheEntry);

		return renderPass;
	}
} // namespace spall::vk
