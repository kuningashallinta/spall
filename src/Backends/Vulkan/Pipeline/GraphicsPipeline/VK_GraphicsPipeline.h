#pragma once

#include <spall/Common/Enums/ShaderStageFlags.h>
#include <spall/Common/Limits.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/VertexInput/VertexBindingInfo.h>
#include <spall/RenderPass/RenderPassBeginInfo.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>
#include <vector>

namespace spall::vk
{
	class Device;
	class CommandList;
	class GraphicsQueue;
	class ResourceSetLayout;

	class GraphicsPipeline final : public SharedObject<IPipeline>
	{
	public:
		struct Bindings
		{
			std::vector<VertexBindingInfo> VertexBindings;
			std::vector<Resource<ResourceSetLayout>> ResourceSetLayouts;
		};

		struct Targets
		{
			const Format* ColorFormats = nullptr;
			std::uint32_t ColorFormatCount = 0;
			Format DepthStencilFormat = Format::Unknown;
			std::uint32_t SampleCount = 1;
			std::uint8_t StencilReference = 0;
		};

		struct PushConstants
		{
			ShaderStageFlags Stages = ShaderStageFlags::None;
			std::uint32_t Size = 0;
		};

		GraphicsPipeline(
			Device& device,
			VkPipelineLayout pipelineLayout,
			VkPipeline pipeline,
			Bindings bindings,
			const Targets& targets,
			const PushConstants& pushConstants);

		~GraphicsPipeline(void) override;

		RenderBackendType backendType(void) const override;
		PipelineType type(void) const override;

	private:
		Resource<Device> m_Device;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		std::vector<VertexBindingInfo> m_VertexBindings;
		std::vector<Resource<ResourceSetLayout>> m_ResourceSetLayouts;

		Format m_ColorTargetFormats[MaxColorAttachments] = {};
		std::uint32_t m_ColorTargetFormatCount = 0;
		Format m_DepthStencilFormat = Format::Unknown;
		std::uint32_t m_SampleCount = 1;
		std::uint8_t m_StencilReference = 0;
		ShaderStageFlags m_PushConstantStages = ShaderStageFlags::None;
		std::uint32_t m_PushConstantSize = 0;

	private:
		friend class Device;
		friend class CommandList;
		friend class GraphicsQueue;
	};
} // namespace spall::vk
