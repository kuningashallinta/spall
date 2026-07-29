#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>

#include <cstdint>

namespace spall
{
	struct BufferInfo
	{
		std::uint32_t Size = 0;
		BufferUsageFlags Usage = BufferUsageFlags::None;
		MemoryAccess CpuAccess = MemoryAccess::None;
		ResourceStateFlags InitialState = ResourceStateFlags::Common;
		bool KeepInitialState = false;
		const char* DebugName = nullptr;
	};
} // namespace spall
