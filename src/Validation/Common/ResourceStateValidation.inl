// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Status validateBufferResourceState(
		const BufferInfo& info,
		ResourceStateFlags state)
	{
		const std::uint32_t value = static_cast<std::uint32_t>(state);
		const std::uint32_t common = static_cast<std::uint32_t>(ResourceStateFlags::Common);
		const std::uint32_t knownStates =
			common |
			static_cast<std::uint32_t>(ResourceStateFlags::VertexBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::IndexBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource) |
			static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopySource) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopyDest) |
			static_cast<std::uint32_t>(ResourceStateFlags::IndirectArgument);

		if (value == common)
		{
			return {};
		}

		if ((value == 0) or ((value & ~knownStates) != 0) or ((value & common) != 0))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		const bool vertexValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::VertexBuffer)) == 0) or
			((info.Usage & BufferUsageFlags::Vertex) != BufferUsageFlags::None);
		const bool indexValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::IndexBuffer)) == 0) or
			((info.Usage & BufferUsageFlags::Index) != BufferUsageFlags::None);
		const bool constantValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer)) == 0) or
			((info.Usage & BufferUsageFlags::Uniform) != BufferUsageFlags::None);
		const bool shaderResourceValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource)) == 0) or
			((info.Usage & BufferUsageFlags::Storage) != BufferUsageFlags::None) or
			((info.Usage & BufferUsageFlags::AccelerationStructureInput) != BufferUsageFlags::None);
		const bool storageValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess)) == 0) or
			((info.Usage & BufferUsageFlags::Storage) != BufferUsageFlags::None);
		const bool sourceValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::CopySource)) == 0) or
			((info.Usage & BufferUsageFlags::TransferSource) != BufferUsageFlags::None);
		const bool destinationValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::CopyDest)) == 0) or
			((info.Usage & BufferUsageFlags::TransferDestination) != BufferUsageFlags::None);
		const bool indirectValid = ((value & static_cast<std::uint32_t>(ResourceStateFlags::IndirectArgument)) == 0) or
			((info.Usage & BufferUsageFlags::Indirect) != BufferUsageFlags::None);

		if (not(vertexValid and indexValid and constantValid and shaderResourceValid and storageValid and sourceValid and destinationValid and indirectValid))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		return {};
	}

	inline Status validateTextureResourceState(
		const TextureInfo& info,
		ResourceStateFlags state,
		bool presentable)
	{
		switch (state)
		{
			case ResourceStateFlags::Common:
			{
				return {};
			}

			case ResourceStateFlags::RenderTarget:
			{
				if ((info.Usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None)
				{
					return {};
				}

				break;
			}

			case ResourceStateFlags::DepthWrite:
			case ResourceStateFlags::DepthRead:
			{
				if ((info.Usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)
				{
					return {};
				}

				break;
			}

			case ResourceStateFlags::ShaderResource:
			{
				if ((info.Usage & TextureUsageFlags::Sampled) != TextureUsageFlags::None)
				{
					return {};
				}

				break;
			}

			case ResourceStateFlags::UnorderedAccess:
			{
				if ((info.Usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
				{
					return {};
				}

				break;
			}

			case ResourceStateFlags::CopySource:
			{
				if ((info.Usage & TextureUsageFlags::TransferSource) != TextureUsageFlags::None)
				{
					return {};
				}

				break;
			}

			case ResourceStateFlags::CopyDest:
			{
				if ((info.Usage & TextureUsageFlags::TransferDestination) != TextureUsageFlags::None)
				{
					return {};
				}

				break;
			}

			case ResourceStateFlags::Present:
			{
				if (presentable)
				{
					return {};
				}

				break;
			}

			default:
			{
				break;
			}
		}

		return ERR_INVALID_RESOURCE_STATE;
	}
} // namespace spall
