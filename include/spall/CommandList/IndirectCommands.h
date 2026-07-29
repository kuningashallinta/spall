#pragma once

#include <cstdint>

namespace spall
{
	/// Layout of the indirect arguments consumed by drawIndirect.
	struct DrawIndirectCommand
	{
		std::uint32_t VertexCount = 0;
		std::uint32_t InstanceCount = 0;
		std::uint32_t StartVertex = 0;
		std::uint32_t StartInstance = 0;
	};

	/// Layout of the indirect arguments consumed by drawIndexedIndirect.
	struct DrawIndexedIndirectCommand
	{
		std::uint32_t IndexCount = 0;
		std::uint32_t InstanceCount = 0;
		std::uint32_t StartIndex = 0;
		std::int32_t VertexOffset = 0;
		std::uint32_t StartInstance = 0;
	};

	/// Layout of the indirect arguments consumed by dispatchIndirect.
	struct DispatchIndirectCommand
	{
		std::uint32_t GroupCountX = 0;
		std::uint32_t GroupCountY = 0;
		std::uint32_t GroupCountZ = 0;
	};

	constexpr std::uint32_t IndirectArgumentAlignment = 4;
} // namespace spall
