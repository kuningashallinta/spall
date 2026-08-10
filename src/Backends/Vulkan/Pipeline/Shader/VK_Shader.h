// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>
#include <vector>

namespace spall::vk
{
	class Device;
	class GraphicsPipeline;

	class Shader final : public SharedObject<IShader>
	{
	public:
		Shader(
			Device& device,
			ShaderStage stage,
			std::vector<std::uint32_t> bytecode,
			VkShaderModule shaderModule);

		~Shader(void) override;

		RenderBackendType backendType(void) const override;

	private:
		Resource<Device> m_Device;

		ShaderStage m_Stage = ShaderStage::Vertex;

		std::vector<std::uint32_t> m_Bytecode;
		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;

	private:
		friend class Device;
		friend class GraphicsPipeline;
	};
} // namespace spall::vk
