#include <src/Backends/Vulkan/Pipeline/ComputePipeline/VK_ComputePipeline.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>

#include <utility>

namespace spall::vk
{
	ComputePipeline::ComputePipeline(
		Device& device,
		VkPipelineLayout pipelineLayout,
		VkPipeline pipeline,
		std::vector<Resource<ResourceSetLayout>> resourceSetLayouts,
		ShaderStageFlags pushConstantStages,
		std::uint32_t pushConstantSize)
		: m_Device(&device), m_PipelineLayout(pipelineLayout), m_Pipeline(pipeline), m_ResourceSetLayouts(std::move(resourceSetLayouts)), m_PushConstantStages(pushConstantStages), m_PushConstantSize(pushConstantSize)
	{
	}

	ComputePipeline::~ComputePipeline()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
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

	RenderBackendType ComputePipeline::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	PipelineType ComputePipeline::type() const
	{
		return PipelineType::Compute;
	}
} // namespace spall::vk
