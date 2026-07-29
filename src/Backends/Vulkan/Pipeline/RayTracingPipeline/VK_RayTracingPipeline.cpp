#include <src/Backends/Vulkan/Pipeline/RayTracingPipeline/VK_RayTracingPipeline.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>

#include <utility>

namespace spall::vk
{
	RayTracingPipeline::RayTracingPipeline(
		Device& device,
		VkPipelineLayout pipelineLayout,
		VkPipeline pipeline,
		std::vector<Resource<ResourceSetLayout>> resourceSetLayouts,
		ShaderStageFlags pushConstantStages,
		std::uint32_t pushConstantSize)
		: m_Device(&device), m_PipelineLayout(pipelineLayout), m_Pipeline(pipeline), m_ResourceSetLayouts(std::move(resourceSetLayouts)), m_PushConstantStages(pushConstantStages), m_PushConstantSize(pushConstantSize)
	{
	}

	RayTracingPipeline::~RayTracingPipeline()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
		}

		if (m_ShaderBindingTable != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Device->m_Allocator, m_ShaderBindingTable, m_ShaderBindingTableAllocation);
		}

		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_Device->m_Device, m_Pipeline, nullptr);
		}

		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_Device->m_Device, m_PipelineLayout, nullptr);
		}
	}

	RenderBackendType RayTracingPipeline::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	PipelineType RayTracingPipeline::type() const
	{
		return PipelineType::RayTracing;
	}
} // namespace spall::vk
