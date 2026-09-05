// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline bool isAccelerationStructureInput(
		const IBuffer& buffer)
	{
		return hasAnyFlag(buffer.info().Usage, BufferUsageFlags::AccelerationStructureInput);
	}

	inline Status validateAccelerationStructureGeometry(
		const AccelerationStructureGeometry& geometry)
	{
		constexpr AccelerationStructureGeometryFlags knownFlags =
			AccelerationStructureGeometryFlags::Opaque |
			AccelerationStructureGeometryFlags::NoDuplicateAnyHitInvocation;

		if (not hasOnlyKnownFlags(geometry.Flags, knownFlags))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (geometry.Type == AccelerationStructureGeometryType::Aabbs)
		{
			if (geometry.AabbBuffer == nullptr)
			{
				return ERR_INVALID_RESOURCE;
			}

			if (not isAccelerationStructureInput(*geometry.AabbBuffer))
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			if (geometry.AabbCount == 0)
			{
				return ERR_INVALID_SIZE;
			}

			if ((geometry.AabbStride < sizeof(AccelerationStructureAabb)) or
				((geometry.AabbStride % AccelerationStructureAabbAlignment) != 0))
			{
				return ERR_INVALID_SIZE;
			}

			if ((geometry.AabbOffset % AccelerationStructureAabbAlignment) != 0)
			{
				return ERR_INVALID_RANGE;
			}

			const std::uint64_t aabbEnd = static_cast<std::uint64_t>(geometry.AabbOffset) +
				(static_cast<std::uint64_t>(geometry.AabbStride) * (static_cast<std::uint64_t>(geometry.AabbCount) - 1)) +
				sizeof(AccelerationStructureAabb);

			if (aabbEnd > static_cast<std::uint64_t>(geometry.AabbBuffer->info().Size))
			{
				return ERR_INVALID_RANGE;
			}

			return {};
		}

		if (geometry.VertexBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE;
		}

		if (not isAccelerationStructureInput(*geometry.VertexBuffer))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (not isVertexFormat(geometry.VertexFormat))
		{
			return ERR_INVALID_FORMAT;
		}

		if ((geometry.VertexStride == 0) or ((geometry.VertexStride % 4) != 0))
		{
			return ERR_INVALID_SIZE;
		}

		if ((geometry.VertexOffset % 4) != 0)
		{
			return ERR_INVALID_RANGE;
		}

		if (geometry.VertexCount < 3)
		{
			return ERR_INVALID_SIZE;
		}

		const std::uint64_t vertexEnd = static_cast<std::uint64_t>(geometry.VertexOffset) +
			(static_cast<std::uint64_t>(geometry.VertexStride) * (static_cast<std::uint64_t>(geometry.VertexCount) - 1)) +
			static_cast<std::uint64_t>(formatBytesPerPixel(geometry.VertexFormat));

		if (vertexEnd > static_cast<std::uint64_t>(geometry.VertexBuffer->info().Size))
		{
			return ERR_INVALID_RANGE;
		}

		if (geometry.IndexBuffer == nullptr)
		{
			if (geometry.IndexCount != 0)
			{
				return ERR_INVALID_SIZE;
			}

			if ((geometry.VertexCount % 3) != 0)
			{
				return ERR_INVALID_SIZE;
			}
		}
		else
		{
			if (not isAccelerationStructureInput(*geometry.IndexBuffer))
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			if ((geometry.IndexCount < 3) or ((geometry.IndexCount % 3) != 0))
			{
				return ERR_INVALID_SIZE;
			}

			const std::uint64_t indexSize = (geometry.IndexFormat == IndexFormat::UInt16) ? 2 : 4;

			if ((static_cast<std::uint64_t>(geometry.IndexOffset) % indexSize) != 0)
			{
				return ERR_INVALID_RANGE;
			}

			const std::uint64_t indexEnd = static_cast<std::uint64_t>(geometry.IndexOffset) +
				(static_cast<std::uint64_t>(geometry.IndexCount) * indexSize);

			if (indexEnd > static_cast<std::uint64_t>(geometry.IndexBuffer->info().Size))
			{
				return ERR_INVALID_RANGE;
			}
		}

		if (geometry.TransformBuffer != nullptr)
		{
			if (not isAccelerationStructureInput(*geometry.TransformBuffer))
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			if ((geometry.TransformOffset % 16) != 0)
			{
				return ERR_INVALID_RANGE;
			}

			const std::uint64_t transformEnd = static_cast<std::uint64_t>(geometry.TransformOffset) + 48;

			if (transformEnd > static_cast<std::uint64_t>(geometry.TransformBuffer->info().Size))
			{
				return ERR_INVALID_RANGE;
			}
		}

		return {};
	}

	inline Status validateAccelerationStructureCreateInfo(
		const AccelerationStructureCreateInfo& info)
	{
		constexpr AccelerationStructureBuildFlags knownFlags =
			AccelerationStructureBuildFlags::AllowUpdate |
			AccelerationStructureBuildFlags::PreferFastTrace |
			AccelerationStructureBuildFlags::PreferFastBuild |
			AccelerationStructureBuildFlags::MinimizeMemory |
			AccelerationStructureBuildFlags::AllowCompaction;

		if (not hasOnlyKnownFlags(info.Flags, knownFlags))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (hasAnyFlag(info.Flags, AccelerationStructureBuildFlags::PreferFastTrace) and
			hasAnyFlag(info.Flags, AccelerationStructureBuildFlags::PreferFastBuild))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (info.Type == AccelerationStructureType::BottomLevel)
		{
			if (info.Geometries.empty())
			{
				return ERR_INVALID_ARGUMENT;
			}

			if (info.Geometries.size() > MaxAccelerationStructureGeometries)
			{
				return ERR_INVALID_SIZE;
			}

			if ((info.InstanceBuffer != nullptr) or (info.InstanceCount != 0))
			{
				return ERR_INVALID_ARGUMENT;
			}

			for (const AccelerationStructureGeometry& geometry : info.Geometries)
			{
				SPALL_TRY(validateAccelerationStructureGeometry(geometry));
			}

			return {};
		}

		if (not info.Geometries.empty())
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (info.InstanceBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE;
		}

		if (not isAccelerationStructureInput(*info.InstanceBuffer))
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if (info.InstanceCount == 0)
		{
			return ERR_INVALID_SIZE;
		}

		if (info.InstanceCount > MaxAccelerationStructureInstances)
		{
			return ERR_INVALID_SIZE;
		}

		if ((info.InstanceBufferOffset % AccelerationStructureInstanceAlignment) != 0)
		{
			return ERR_INVALID_RANGE;
		}

		const std::uint64_t instanceEnd = static_cast<std::uint64_t>(info.InstanceBufferOffset) +
			(static_cast<std::uint64_t>(info.InstanceCount) * sizeof(AccelerationStructureInstance));

		if (instanceEnd > static_cast<std::uint64_t>(info.InstanceBuffer->info().Size))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateAccelerationStructureBuildInfo(
		const AccelerationStructureInfo& info,
		const AccelerationStructureBuildInfo& buildInfo)
	{
		if (buildInfo.Update and (not hasAnyFlag(info.Flags, AccelerationStructureBuildFlags::AllowUpdate)))
		{
			return ERR_INVALID_STATE;
		}

		if (buildInfo.InstanceCount != 0)
		{
			if (info.Type != AccelerationStructureType::TopLevel)
			{
				return ERR_INVALID_ARGUMENT;
			}

			if (buildInfo.InstanceCount > info.InstanceCount)
			{
				return ERR_INVALID_RANGE;
			}
		}

		return {};
	}

	inline Status validateAccelerationStructureCompaction(
		const AccelerationStructureInfo& info)
	{
		if (not hasAnyFlag(info.Flags, AccelerationStructureBuildFlags::AllowCompaction))
		{
			return ERR_INVALID_STATE;
		}

		return {};
	}
} // namespace spall
