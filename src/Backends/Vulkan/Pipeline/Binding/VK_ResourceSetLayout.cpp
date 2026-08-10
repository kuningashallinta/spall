// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <utility>

namespace spall::vk
{
	ResourceSetLayout::ResourceSetLayout(
		Device& device,
		std::vector<ResourceBindingInfo> bindings,
		VkDescriptorSetLayout descriptorSetLayout)
		: m_Device(&device), m_Bindings(std::move(bindings)), m_DescriptorSetLayout(descriptorSetLayout)
	{
	}

	ResourceSetLayout::~ResourceSetLayout()
	{
		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE) and (m_DescriptorSetLayout != VK_NULL_HANDLE))
		{
			vkDestroyDescriptorSetLayout(m_Device->m_Device, m_DescriptorSetLayout, nullptr);
		}
	}

	RenderBackendType ResourceSetLayout::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	const ResourceBindingInfo* ResourceSetLayout::findBinding(
		std::uint32_t binding) const
	{
		for (const ResourceBindingInfo& bindingInfo : m_Bindings)
		{
			if (bindingInfo.Binding == binding)
			{
				return &bindingInfo;
			}
		}

		return nullptr;
	}
} // namespace spall::vk
