// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <vk_mem_alloc.h>

#include <cstdint>
#include <string>
#include <vector>

namespace spall::vk
{
	class Buffer;
	class CommandList;
	class Device;
	class ResourceSet;

	class AccelerationStructure final : public SharedObject<IAccelerationStructure>
	{
	public:
		AccelerationStructure(
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
			std::uint32_t instanceBufferOffset);

		~AccelerationStructure(void) override;

		RenderBackendType backendType(void) const override;
		AccelerationStructureInfo info(void) const override;
		std::uint64_t deviceAddress(void) const override;

	private:
		Resource<Device> m_Device;

		std::string m_DebugName;
		AccelerationStructureInfo m_Info = {};

		VkAccelerationStructureKHR m_AccelerationStructure = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;

		VkBuffer m_ScratchBuffer = VK_NULL_HANDLE;
		VmaAllocation m_ScratchAllocation = VK_NULL_HANDLE;
		VkDeviceAddress m_ScratchAddress = 0;

		std::vector<Resource<Buffer>> m_InputBuffers;

		std::vector<VkAccelerationStructureGeometryKHR> m_Geometries;
		std::vector<std::uint32_t> m_PrimitiveCounts;

		Resource<Buffer> m_InstanceBuffer;
		std::uint32_t m_InstanceBufferOffset = 0;

		bool m_Built = false;
		std::uint32_t m_BuiltInstanceCount = 0;

		VkQueryPool m_CompactedSizeQueryPool = VK_NULL_HANDLE;
		bool m_Compacted = false;

	private:
		friend class CommandList;
		friend class Device;
		friend class ResourceSet;
	};
} // namespace spall::vk
