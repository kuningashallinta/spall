// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline VkShaderStageFlags vulkanShaderStageFlags(
		ShaderStageFlags stages)
	{
		VkShaderStageFlags flags = 0;

		if ((stages & ShaderStageFlags::Vertex) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_VERTEX_BIT;
		}

		if ((stages & ShaderStageFlags::Fragment) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		if ((stages & ShaderStageFlags::Compute) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_COMPUTE_BIT;
		}

		if ((stages & ShaderStageFlags::Geometry) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		}

		if ((stages & ShaderStageFlags::TessellationControl) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		}

		if ((stages & ShaderStageFlags::TessellationEvaluation) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		}

		if ((stages & ShaderStageFlags::RayGeneration) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		}

		if ((stages & ShaderStageFlags::Miss) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
		}

		if ((stages & ShaderStageFlags::ClosestHit) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		}

		if ((stages & ShaderStageFlags::AnyHit) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		}

		if ((stages & ShaderStageFlags::Intersection) != ShaderStageFlags::None)
		{
			flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		}

		return flags;
	}

	inline VkDescriptorType vulkanDescriptorType(
		ResourceBindingType type)
	{
		switch (type)
		{
			case ResourceBindingType::UniformBuffer:
			{
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}

			case ResourceBindingType::StorageBuffer:
			{
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			case ResourceBindingType::StorageTexture:
			{
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}

			case ResourceBindingType::AccelerationStructure:
			{
				return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			}

			case ResourceBindingType::SampledTexture:
			default:
			{
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}
		}
	}
} // namespace spall::vk
