#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#include <cstdint>

namespace spall
{
	/// Describes a buffer to be created by a resource factory.
	struct BufferCreateInfo
	{
		/// Size of the buffer, in bytes.
		std::uint32_t Size = 0;

		BufferUsageFlags Usage = BufferUsageFlags::None;
		MemoryAccess CpuAccess = MemoryAccess::None;

		/// State reported by the buffer immediately after creation.
		///
		/// Compatible buffer-access states may be combined.
		ResourceStateFlags InitialState = ResourceStateFlags::Common;

		/// Restores InitialState after an automatic copy transition.
		bool KeepInitialState = false;

		const char* DebugName = nullptr;
	};
} // namespace spall
