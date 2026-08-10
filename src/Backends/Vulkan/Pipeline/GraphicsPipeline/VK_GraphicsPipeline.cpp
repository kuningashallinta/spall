// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Pipeline/GraphicsPipeline/VK_GraphicsPipeline.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>

#include <cstdint>

namespace spall::vk
{
	GraphicsPipeline::GraphicsPipeline(
		Device& device,
		VkPipelineLayout pipelineLayout,
		VkPipeline pipeline,
		Bindings bindings,
		const Targets& targets,
		const PushConstants& pushConstants)
		: m_Device(&device),
		  m_PipelineLayout(pipelineLayout),
		  m_Pipeline(pipeline),
		  m_VertexBindings(std::move(bindings.VertexBindings)),
		  m_ResourceSetLayouts(std::move(bindings.ResourceSetLayouts)),
		  m_ColorTargetFormatCount((std::min)(targets.ColorFormatCount, MaxColorAttachments)),
		  m_DepthStencilFormat(targets.DepthStencilFormat),
		  m_SampleCount(targets.SampleCount),
		  m_StencilReference(targets.StencilReference),
		  m_PushConstantStages(pushConstants.Stages),
		  m_PushConstantSize(pushConstants.Size)
	{
		for (std::uint32_t formatIndex = 0; formatIndex < m_ColorTargetFormatCount; ++formatIndex)
		{
			m_ColorTargetFormats[formatIndex] = targets.ColorFormats[formatIndex];
		}
	}

	GraphicsPipeline::~GraphicsPipeline()
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

	RenderBackendType GraphicsPipeline::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	PipelineType GraphicsPipeline::type() const
	{
		return PipelineType::Graphics;
	}
} // namespace spall::vk
