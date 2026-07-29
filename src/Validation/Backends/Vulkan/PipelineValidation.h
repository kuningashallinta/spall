#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>
#include <spall/Pipeline/Pipeline/PipelineCreateInfo.h>

#include <vulkan/vulkan.hpp>

#include <span>

namespace spall::vk
{
	class PipelineValidation
	{
	public:
		static Status validateCreateInfo(
			const PipelineCreateInfo& info,
			const VkPhysicalDeviceFeatures& features,
			const VkPhysicalDeviceLimits& limits);

		static Status validateDescriptorBindings(
			std::span<const ResourceBindingInfo> bindings,
			const VkPhysicalDeviceLimits& limits);
	};
} // namespace spall::vk
