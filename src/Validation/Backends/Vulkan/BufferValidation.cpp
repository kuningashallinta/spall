#include <src/Validation/Backends/Vulkan/BufferValidation.h>

namespace spall::vk
{
	Status BufferValidation::validateCreateInfo(
		const BufferCreateInfo& info,
		const VkPhysicalDeviceLimits& limits)
	{
		if (((info.Usage & BufferUsageFlags::Uniform) != BufferUsageFlags::None) and
			(info.Size > limits.maxUniformBufferRange))
		{
			return ERR_INVALID_SIZE;
		}

		return {};
	}
} // namespace spall::vk
