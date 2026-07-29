#include <src/Backends/Vulkan/Resources/Query/VK_QueryPool.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Common/VK_DebugName.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>

namespace spall::vk
{
	QueryPool::QueryPool(
		Device& device,
		const QueryPoolInfo& info,
		VkQueryPool queryPool)
		: m_Device(&device), m_DebugName(info.DebugName != nullptr ? info.DebugName : ""), m_Info(info), m_QueryPool(queryPool)
	{
		m_Info.DebugName = m_DebugName.empty() ? nullptr : m_DebugName.c_str();
		setDebugName(
			m_Device->m_Device,
			VK_OBJECT_TYPE_QUERY_POOL,
			reinterpret_cast<std::uint64_t>(m_QueryPool),
			m_Info.DebugName);
	}

	QueryPool::~QueryPool()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
		}

		if (m_QueryPool != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(m_Device->m_Device, m_QueryPool, nullptr);
		}
	}

	RenderBackendType QueryPool::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	QueryPoolInfo QueryPool::info() const
	{
		return m_Info;
	}
} // namespace spall::vk
