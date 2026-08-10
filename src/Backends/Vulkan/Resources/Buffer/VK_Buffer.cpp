// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Resources/Buffer/VK_Buffer.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Common/VK_DebugName.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>

namespace spall::vk
{
	Buffer::Buffer(
		Device& device,
		const BufferInfo& info,
		VkBuffer buffer,
		VmaAllocation allocation,
		ResourceStateFlags currentState)
		: m_Device(&device), m_DebugName(info.DebugName != nullptr ? info.DebugName : ""), m_Info(info), m_Buffer(buffer), m_Allocation(allocation), m_CurrentState(currentState)
	{
		m_Info.DebugName = m_DebugName.empty() ? nullptr : m_DebugName.c_str();
		setDebugName(
			m_Device->m_Device,
			VK_OBJECT_TYPE_BUFFER,
			reinterpret_cast<std::uint64_t>(m_Buffer),
			m_Info.DebugName);
	}

	Buffer::~Buffer()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
		}

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Device->m_Allocator, m_Buffer, m_Allocation);
		}
	}

	RenderBackendType Buffer::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	BufferInfo Buffer::info() const
	{
		return m_Info;
	}
} // namespace spall::vk
