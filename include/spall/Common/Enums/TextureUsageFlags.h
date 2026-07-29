#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies the operations for which a texture is created.
	enum class TextureUsageFlags : std::uint32_t
	{
		None = 0,
		ColorAttachment = BIT(0),
		DepthStencilAttachment = BIT(1),
		TransferSource = BIT(2),
		TransferDestination = BIT(3),
		Sampled = BIT(4),
		Storage = BIT(5)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		TextureUsageFlags)
} // namespace spall
