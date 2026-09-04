// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

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
		Common = SPALL_BIT(0),
		VertexBuffer = SPALL_BIT(1),
		IndexBuffer = SPALL_BIT(2),
		ConstantBuffer = SPALL_BIT(3),
		ShaderResource = SPALL_BIT(5),
		UnorderedAccess = SPALL_BIT(6),
		RenderTarget = SPALL_BIT(7),
		DepthWrite = SPALL_BIT(8),
		DepthRead = SPALL_BIT(9),
		CopySource = SPALL_BIT(10),
		CopyDest = SPALL_BIT(11),
		Present = SPALL_BIT(12),
		IndirectArgument = SPALL_BIT(13)
	};

	SPALL_ENUM_CLASS_BITWISE_OPERATORS(
		ResourceStateFlags)
} // namespace spall
