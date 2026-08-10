// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Pipeline/Shader/VK_Shader.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <cstdint>

namespace spall::vk
{
	Shader::Shader(
		Device& device,
		ShaderStage stage,
		std::vector<std::uint32_t> bytecode,
		VkShaderModule shaderModule)
		: m_Device(&device), m_Stage(stage), m_Bytecode(std::move(bytecode)), m_ShaderModule(shaderModule)
	{
	}

	Shader::~Shader()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
		}

		if (m_ShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(m_Device->m_Device, m_ShaderModule, nullptr);
		}
	}

	RenderBackendType Shader::backendType() const
	{
		return RenderBackendType::Vulkan;
	}
} // namespace spall::vk
