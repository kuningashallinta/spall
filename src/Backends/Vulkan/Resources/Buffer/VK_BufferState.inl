// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline std::optional<BufferStateInfo> bufferState(
		ResourceStateFlags state)
	{
		const std::uint32_t stateValue = static_cast<std::uint32_t>(state);

		constexpr std::uint32_t supportedStateMask =
			static_cast<std::uint32_t>(ResourceStateFlags::Common) |
			static_cast<std::uint32_t>(ResourceStateFlags::VertexBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::IndexBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource) |
			static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopySource) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopyDest) |
			static_cast<std::uint32_t>(ResourceStateFlags::IndirectArgument);

		if ((stateValue == 0) or ((stateValue & ~supportedStateMask) != 0) or
			(((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::Common)) != 0) and
				(state != ResourceStateFlags::Common)))
		{
			return std::nullopt;
		}

		VkAccessFlags access = 0;
		VkPipelineStageFlags stage = 0;

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::Common)) != 0)
		{
			access |= VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
			stage |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::VertexBuffer)) != 0)
		{
			access |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			stage |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::IndexBuffer)) != 0)
		{
			access |= VK_ACCESS_INDEX_READ_BIT;
			stage |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer)) != 0)
		{
			access |= VK_ACCESS_UNIFORM_READ_BIT;
			stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource)) != 0)
		{
			access |= VK_ACCESS_SHADER_READ_BIT;
			stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess)) != 0)
		{
			access |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::CopySource)) != 0)
		{
			access |= VK_ACCESS_TRANSFER_READ_BIT;
			stage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::CopyDest)) != 0)
		{
			access |= VK_ACCESS_TRANSFER_WRITE_BIT;
			stage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if ((stateValue & static_cast<std::uint32_t>(ResourceStateFlags::IndirectArgument)) != 0)
		{
			access |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			stage |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		}

		if (stage == 0)
		{
			return std::nullopt;
		}

		return BufferStateInfo {access, stage};
	}
} // namespace spall::vk
