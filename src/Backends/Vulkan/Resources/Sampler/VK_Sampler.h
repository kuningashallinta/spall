// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/Sampler/ISampler.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

namespace spall::vk
{
	class Device;
	class GraphicsQueue;
	class CommandList;
	class ResourceSet;

	class Sampler final : public SharedObject<ISampler>
	{
	public:
		Sampler(
			Device& device,
			VkSampler sampler);

		~Sampler(void) override;

		RenderBackendType backendType(void) const override;

	private:
		Resource<Device> m_Device;
		VkSampler m_Sampler = VK_NULL_HANDLE;

	private:
		friend class Device;
		friend class GraphicsQueue;
		friend class CommandList;
		friend class ResourceSet;
	};
} // namespace spall::vk
