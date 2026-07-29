#include <src/Backends/Vulkan/Resources/Sampler/VK_Sampler.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>

namespace spall::vk
{
	Sampler::Sampler(
		Device& device,
		VkSampler sampler)
		: m_Device(&device), m_Sampler(sampler)
	{
	}

	Sampler::~Sampler()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
		}

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device->m_Device, m_Sampler, nullptr);
		}
	}

	RenderBackendType Sampler::backendType() const
	{
		return RenderBackendType::Vulkan;
	}
} // namespace spall::vk
