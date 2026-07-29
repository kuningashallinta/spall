#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>
#include <vector>

namespace spall::vk
{
	class Device;
	class CommandList;
	class ResourceSet;

	class ResourceSetLayout final : public SharedObject<IResourceSetLayout>
	{
	public:
		ResourceSetLayout(
			Device& device,
			std::vector<ResourceBindingInfo> bindings,
			VkDescriptorSetLayout descriptorSetLayout);

		~ResourceSetLayout(void) override;

		RenderBackendType backendType(void) const override;

	private:
		const ResourceBindingInfo* findBinding(std::uint32_t binding) const;

		Resource<Device> m_Device;
		std::vector<ResourceBindingInfo> m_Bindings;
		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;

	private:
		friend class Device;
		friend class CommandList;
		friend class ResourceSet;
	};
} // namespace spall::vk
