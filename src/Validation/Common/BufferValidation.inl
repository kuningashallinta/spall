namespace spall
{
	inline Status validateBufferUsage(
		const BufferCreateInfo& info)
	{
		constexpr BufferUsageFlags knownUsage =
			BufferUsageFlags::Vertex |
			BufferUsageFlags::Index |
			BufferUsageFlags::Uniform |
			BufferUsageFlags::TransferSource |
			BufferUsageFlags::TransferDestination |
			BufferUsageFlags::Storage |
			BufferUsageFlags::Indirect |
			BufferUsageFlags::AccelerationStructureInput;

		if (info.Size == 0)
		{
			return ERR_INVALID_SIZE;
		}

		if (info.Usage == BufferUsageFlags::None)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (not hasOnlyKnownFlags(info.Usage, knownUsage))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (hasAnyFlag(info.Usage, BufferUsageFlags::Indirect) and (info.CpuAccess != MemoryAccess::None))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		if (hasAnyFlag(info.Usage, BufferUsageFlags::AccelerationStructureInput) and (info.CpuAccess == MemoryAccess::Read))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		return {};
	}

	inline Status validateBufferInitialState(
		const BufferCreateInfo& info)
	{
		constexpr ResourceStateFlags knownState = static_cast<ResourceStateFlags>(
			static_cast<std::uint32_t>(ResourceStateFlags::Common) |
			static_cast<std::uint32_t>(ResourceStateFlags::VertexBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::IndexBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::ConstantBuffer) |
			static_cast<std::uint32_t>(ResourceStateFlags::ShaderResource) |
			static_cast<std::uint32_t>(ResourceStateFlags::UnorderedAccess) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopySource) |
			static_cast<std::uint32_t>(ResourceStateFlags::CopyDest) |
			static_cast<std::uint32_t>(ResourceStateFlags::IndirectArgument));

		const std::uint32_t initialState = static_cast<std::uint32_t>(info.InitialState);

		if ((initialState == 0) or (not hasOnlyKnownFlags(info.InitialState, knownState)) or
			(((initialState & static_cast<std::uint32_t>(ResourceStateFlags::Common)) != 0) and
				(info.InitialState != ResourceStateFlags::Common)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::VertexBuffer) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::Vertex)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::IndexBuffer) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::Index)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::ConstantBuffer) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::Uniform)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::ShaderResource) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::Storage)) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::AccelerationStructureInput)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::UnorderedAccess) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::Storage)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::CopySource) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::TransferSource)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::CopyDest) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::TransferDestination)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (hasAnyFlag(info.InitialState, ResourceStateFlags::IndirectArgument) and
			(not hasAnyFlag(info.Usage, BufferUsageFlags::Indirect)))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		return {};
	}

	inline Status validateBufferCreateInfo(
		const BufferCreateInfo& info)
	{
		SPALL_TRY(validateBufferUsage(info));
		SPALL_TRY(validateBufferInitialState(info));

		return {};
	}
} // namespace spall
