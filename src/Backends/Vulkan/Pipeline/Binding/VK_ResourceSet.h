// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Binding/IResourceSet.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>
#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSetLayout.h>

#include <cstdint>
#include <span>
#include <vector>

namespace spall::vk
{
	class Device;
	class CommandList;

	class ResourceSet final : public SharedObject<IResourceSet>
	{
	public:
		ResourceSet(
			Device& device,
			ResourceSetLayout& layout,
			VkDescriptorPool descriptorPool,
			VkDescriptorSet descriptorSet);

		~ResourceSet(void) override;

		RenderBackendType backendType(void) const override;

		IResourceSetLayout& layout(void) const override;

		Status writeResources(std::span<const ResourceWrite> writes) override;

	private:
		const ResourceWrite* findWrite(std::uint32_t binding) const;

		Resource<Device> m_Device;
		Resource<ResourceSetLayout> m_Layout;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		std::vector<ResourceWrite> m_Writes;
		std::vector<Resource<IResource>> m_RetainedResources;
		std::uint32_t m_CommandListReferenceCount = 0;

	private:
		friend class CommandList;
	};
} // namespace spall::vk
