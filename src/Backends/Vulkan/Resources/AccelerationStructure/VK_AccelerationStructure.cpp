// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Resources/AccelerationStructure/VK_AccelerationStructure.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Common/VK_DebugName.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_Buffer.h>

#include <utility>

namespace spall::vk
{
	AccelerationStructure::AccelerationStructure(
		Device& device,
		const AccelerationStructureInfo& info,
		VkAccelerationStructureKHR accelerationStructure,
		VkBuffer buffer,
		VmaAllocation allocation,
		VkBuffer scratchBuffer,
		VmaAllocation scratchAllocation,
		VkDeviceAddress scratchAddress,
		std::vector<Resource<Buffer>> inputBuffers,
		std::vector<VkAccelerationStructureGeometryKHR> geometries,
		std::vector<std::uint32_t> primitiveCounts,
		Resource<Buffer> instanceBuffer,
		std::uint32_t instanceBufferOffset)
		: m_Device(&device), m_DebugName(info.DebugName != nullptr ? info.DebugName : ""), m_Info(info), m_AccelerationStructure(accelerationStructure), m_Buffer(buffer), m_Allocation(allocation), m_ScratchBuffer(scratchBuffer), m_ScratchAllocation(scratchAllocation), m_ScratchAddress(scratchAddress), m_InputBuffers(std::move(inputBuffers)), m_Geometries(std::move(geometries)), m_PrimitiveCounts(std::move(primitiveCounts)), m_InstanceBuffer(std::move(instanceBuffer)), m_InstanceBufferOffset(instanceBufferOffset)
	{
		m_Info.DebugName = m_DebugName.empty() ? nullptr : m_DebugName.c_str();
		setDebugName(
			m_Device->m_Device,
			VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR,
			reinterpret_cast<std::uint64_t>(m_AccelerationStructure),
			m_Info.DebugName);
	}

	AccelerationStructure::~AccelerationStructure()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			return;
		}

		if (m_AccelerationStructure != VK_NULL_HANDLE)
		{
			m_Device->m_DestroyAccelerationStructure(m_Device->m_Device, m_AccelerationStructure, nullptr);
		}

		if (m_CompactedSizeQueryPool != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(m_Device->m_Device, m_CompactedSizeQueryPool, nullptr);
		}

		if (m_ScratchBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Device->m_Allocator, m_ScratchBuffer, m_ScratchAllocation);
		}

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Device->m_Allocator, m_Buffer, m_Allocation);
		}
	}

	RenderBackendType AccelerationStructure::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	AccelerationStructureInfo AccelerationStructure::info() const
	{
		return m_Info;
	}

	std::uint64_t AccelerationStructure::deviceAddress() const
	{
		VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
		addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure = m_AccelerationStructure;

		return m_Device->m_GetAccelerationStructureDeviceAddress(m_Device->m_Device, &addressInfo);
	}
} // namespace spall::vk
