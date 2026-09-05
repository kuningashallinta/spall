// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Validation/Backends/Vulkan/PipelineValidation.h>

#include <cstdint>

namespace spall::vk
{
	Status PipelineValidation::validateCreateInfo(
		const PipelineCreateInfo& info,
		const VkPhysicalDeviceFeatures& features,
		const VkPhysicalDeviceLimits& limits)
	{
		if ((info.FillMode == FillMode::Wireframe) and (features.fillModeNonSolid == VK_FALSE))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if ((info.DepthBiasClamp != 0.0f) and (features.depthBiasClamp == VK_FALSE))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if (((info.TessellationControlShader.Module != nullptr) or (info.TessellationEvaluationShader.Module != nullptr)) and
			(features.tessellationShader == VK_FALSE))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if ((info.GeometryShader.Module != nullptr) and (features.geometryShader == VK_FALSE))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if (info.LineWidth != 1.0f)
		{
			if (features.wideLines == VK_FALSE)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if ((info.LineWidth < limits.lineWidthRange[0]) or (info.LineWidth > limits.lineWidthRange[1]))
			{
				return ERR_INVALID_RANGE;
			}
		}

		if ((info.VertexBindings.size() > limits.maxVertexInputBindings) or
			(info.VertexAttributes.size() > limits.maxVertexInputAttributes) or
			(info.ColorTargetFormatCount > limits.maxColorAttachments))
		{
			return ERR_INVALID_BINDING;
		}

		for (const VertexBindingInfo& binding : info.VertexBindings)
		{
			if ((binding.Binding >= limits.maxVertexInputBindings) or
				(binding.Stride > limits.maxVertexInputBindingStride))
			{
				return ERR_INVALID_BINDING;
			}
		}

		for (const VertexAttributeInfo& attribute : info.VertexAttributes)
		{
			if ((attribute.Location >= limits.maxVertexInputAttributes) or
				(attribute.Offset > limits.maxVertexInputAttributeOffset))
			{
				return ERR_INVALID_BINDING;
			}
		}

		return {};
	}

	Status PipelineValidation::validateDescriptorBindings(
		std::span<const ResourceBindingInfo> bindings,
		const VkPhysicalDeviceLimits& limits)
	{
		std::uint32_t uniformBufferCount = 0;
		std::uint32_t sampledTextureCount = 0;
		std::uint32_t vertexUniformBufferCount = 0;
		std::uint32_t fragmentUniformBufferCount = 0;
		std::uint32_t computeUniformBufferCount = 0;
		std::uint32_t vertexSampledTextureCount = 0;
		std::uint32_t fragmentSampledTextureCount = 0;
		std::uint32_t computeSampledTextureCount = 0;
		std::uint32_t storageBufferCount = 0;
		std::uint32_t computeStorageBufferCount = 0;
		std::uint32_t storageTextureCount = 0;
		std::uint32_t computeStorageTextureCount = 0;

		for (const ResourceBindingInfo& binding : bindings)
		{
			if (binding.Type == ResourceBindingType::UniformBuffer)
			{
				++uniformBufferCount;

				if ((binding.Stages & ShaderStageFlags::Vertex) != ShaderStageFlags::None)
				{
					++vertexUniformBufferCount;
				}

				if ((binding.Stages & ShaderStageFlags::Fragment) != ShaderStageFlags::None)
				{
					++fragmentUniformBufferCount;
				}

				if ((binding.Stages & ShaderStageFlags::Compute) != ShaderStageFlags::None)
				{
					++computeUniformBufferCount;
				}
			}
			else if (binding.Type == ResourceBindingType::SampledTexture)
			{
				++sampledTextureCount;

				if ((binding.Stages & ShaderStageFlags::Vertex) != ShaderStageFlags::None)
				{
					++vertexSampledTextureCount;
				}

				if ((binding.Stages & ShaderStageFlags::Fragment) != ShaderStageFlags::None)
				{
					++fragmentSampledTextureCount;
				}

				if ((binding.Stages & ShaderStageFlags::Compute) != ShaderStageFlags::None)
				{
					++computeSampledTextureCount;
				}
			}
			else if (binding.Type == ResourceBindingType::StorageBuffer)
			{
				++storageBufferCount;

				if ((binding.Stages & ShaderStageFlags::Compute) != ShaderStageFlags::None)
				{
					++computeStorageBufferCount;
				}
			}
			else if (binding.Type == ResourceBindingType::StorageTexture)
			{
				++storageTextureCount;

				if ((binding.Stages & ShaderStageFlags::Compute) != ShaderStageFlags::None)
				{
					++computeStorageTextureCount;
				}
			}
		}

		if ((uniformBufferCount > limits.maxDescriptorSetUniformBuffers) or
			(sampledTextureCount > limits.maxDescriptorSetSampledImages) or
			(sampledTextureCount > limits.maxDescriptorSetSamplers) or
			(vertexUniformBufferCount > limits.maxPerStageDescriptorUniformBuffers) or
			(fragmentUniformBufferCount > limits.maxPerStageDescriptorUniformBuffers) or
			(computeUniformBufferCount > limits.maxPerStageDescriptorUniformBuffers) or
			(vertexSampledTextureCount > limits.maxPerStageDescriptorSampledImages) or
			(fragmentSampledTextureCount > limits.maxPerStageDescriptorSampledImages) or
			(computeSampledTextureCount > limits.maxPerStageDescriptorSampledImages) or
			(vertexSampledTextureCount > limits.maxPerStageDescriptorSamplers) or
			(fragmentSampledTextureCount > limits.maxPerStageDescriptorSamplers) or
			(computeSampledTextureCount > limits.maxPerStageDescriptorSamplers) or
			(storageBufferCount > limits.maxDescriptorSetStorageBuffers) or
			(computeStorageBufferCount > limits.maxPerStageDescriptorStorageBuffers) or
			(storageTextureCount > limits.maxDescriptorSetStorageImages) or
			(computeStorageTextureCount > limits.maxPerStageDescriptorStorageImages) or
			((vertexUniformBufferCount + vertexSampledTextureCount) > limits.maxPerStageResources) or
			((fragmentUniformBufferCount + fragmentSampledTextureCount) > limits.maxPerStageResources) or
			((computeUniformBufferCount + computeSampledTextureCount + computeStorageBufferCount + computeStorageTextureCount) > limits.maxPerStageResources))
		{
			return ERR_INVALID_BINDING;
		}

		return {};
	}
} // namespace spall::vk
