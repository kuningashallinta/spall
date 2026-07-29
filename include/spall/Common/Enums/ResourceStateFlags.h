#pragma once

#include <spall/Common/Bit.h>

#include <cstdint>

namespace spall
{
	/// Identifies how a resource is accessed by recorded GPU work.
	///
	/// Textures use one state at a time. Buffers may combine compatible
	/// buffer-access states.
	enum class ResourceStateFlags : std::uint32_t
	{
		Unknown = 0,
		Common = BIT(0),
		VertexBuffer = BIT(1),
		IndexBuffer = BIT(2),
		ConstantBuffer = BIT(3),
		ShaderResource = BIT(5),
		UnorderedAccess = BIT(6),
		RenderTarget = BIT(7),
		DepthWrite = BIT(8),
		DepthRead = BIT(9),
		CopySource = BIT(10),
		CopyDest = BIT(11),
		Present = BIT(12),
		IndirectArgument = BIT(13)
	};

	ENUM_CLASS_BITWISE_OPERATORS(
		ResourceStateFlags)
} // namespace spall
