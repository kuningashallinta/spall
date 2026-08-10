// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ShaderStageFlags.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>
#include <vector>

namespace spall::vk
{
	class Device;
	class CommandList;
	class GraphicsQueue;
	class ResourceSetLayout;

	class ComputePipeline final : public SharedObject<IPipeline>
	{
	public:
		ComputePipeline(
			Device& device,
			VkPipelineLayout pipelineLayout,
			VkPipeline pipeline,
			std::vector<Resource<ResourceSetLayout>> resourceSetLayouts,
			ShaderStageFlags pushConstantStages,
			std::uint32_t pushConstantSize);

		~ComputePipeline(void) override;

		RenderBackendType backendType(void) const override;
		PipelineType type(void) const override;

	private:
		Resource<Device> m_Device;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		std::vector<Resource<ResourceSetLayout>> m_ResourceSetLayouts;
		ShaderStageFlags m_PushConstantStages = ShaderStageFlags::None;
		std::uint32_t m_PushConstantSize = 0;

	private:
		friend class Device;
		friend class CommandList;
		friend class GraphicsQueue;
	};
} // namespace spall::vk
