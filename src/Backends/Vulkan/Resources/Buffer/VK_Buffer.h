// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/Buffer/IBuffer.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <vk_mem_alloc.h>

#include <string>

namespace spall::vk
{
	class Device;
	class GraphicsQueue;
	class CommandList;
	class ResourceSet;

	class Buffer final : public SharedObject<IBuffer>
	{
	public:
		Buffer(
			Device& device,
			const BufferInfo& info,
			VkBuffer buffer,
			VmaAllocation allocation,
			ResourceStateFlags currentState);

		~Buffer(void) override;

		RenderBackendType backendType(void) const override;
		BufferInfo info(void) const override;

	private:
		Resource<Device> m_Device;
		std::string m_DebugName;
		BufferInfo m_Info = {};
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		ResourceStateFlags m_CurrentState = ResourceStateFlags::Unknown;
		ResourceStateFlags m_PermanentState = ResourceStateFlags::Unknown;

	private:
		friend class Device;
		friend class GraphicsQueue;
		friend class CommandList;
		friend class ResourceStateTracker;
		friend class ResourceSet;
	};
} // namespace spall::vk
