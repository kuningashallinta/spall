// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline VkBufferUsageFlags bufferUsageFlags(
		BufferUsageFlags usage,
		bool rayTracingEnabled)
	{
		VkBufferUsageFlags bufferUsage = 0;

		if ((usage & BufferUsageFlags::Vertex) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::Index) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::Uniform) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::TransferSource) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}

		if ((usage & BufferUsageFlags::TransferDestination) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		if ((usage & BufferUsageFlags::Storage) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::Indirect) != BufferUsageFlags::None)
		{
			bufferUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::AccelerationStructureInput) != BufferUsageFlags::None)
		{
			bufferUsage |= rayTracingEnabled
				? (VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
				: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}

		return bufferUsage;
	}
} // namespace spall::vk
