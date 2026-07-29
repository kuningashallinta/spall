namespace spall::d3d12
{
	inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE nativeAccelerationStructureType(
		AccelerationStructureType type)
	{
		return (type == AccelerationStructureType::TopLevel)
			? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL
			: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	}

	inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nativeAccelerationStructureBuildFlags(
		AccelerationStructureBuildFlags flags)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nativeFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		if ((flags & AccelerationStructureBuildFlags::AllowUpdate) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		}

		if ((flags & AccelerationStructureBuildFlags::PreferFastTrace) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		}

		if ((flags & AccelerationStructureBuildFlags::PreferFastBuild) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		}

		if ((flags & AccelerationStructureBuildFlags::MinimizeMemory) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		}

		if ((flags & AccelerationStructureBuildFlags::AllowCompaction) != AccelerationStructureBuildFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		}

		return nativeFlags;
	}

	inline D3D12_RAYTRACING_GEOMETRY_FLAGS nativeAccelerationStructureGeometryFlags(
		AccelerationStructureGeometryFlags flags)
	{
		D3D12_RAYTRACING_GEOMETRY_FLAGS nativeFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

		if ((flags & AccelerationStructureGeometryFlags::Opaque) != AccelerationStructureGeometryFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}

		if ((flags & AccelerationStructureGeometryFlags::NoDuplicateAnyHitInvocation) != AccelerationStructureGeometryFlags::None)
		{
			nativeFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
		}

		return nativeFlags;
	}

	inline bool isSupportedAccelerationStructureVertexFormat(
		Format format)
	{
		return (format == Format::RGB32Float) or (format == Format::RG32Float) or
			(format == Format::RGBA16Float) or (format == Format::RG16Float) or
			(format == Format::RGBA16Snorm) or (format == Format::RG16Snorm);
	}

	inline D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS accelerationStructureInputs(
		AccelerationStructureType type,
		AccelerationStructureBuildFlags flags,
		std::span<const D3D12_RAYTRACING_GEOMETRY_DESC> geometries,
		std::uint32_t instanceCount,
		D3D12_GPU_VIRTUAL_ADDRESS instanceAddress)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.Type = nativeAccelerationStructureType(type);
		inputs.Flags = nativeAccelerationStructureBuildFlags(flags);
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		if (type == AccelerationStructureType::TopLevel)
		{
			inputs.NumDescs = instanceCount;
			inputs.InstanceDescs = instanceAddress;
		}
		else
		{
			inputs.NumDescs = static_cast<UINT>(geometries.size());
			inputs.pGeometryDescs = geometries.data();
		}

		return inputs;
	}
} // namespace spall::d3d12
