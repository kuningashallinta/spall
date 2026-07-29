#pragma once

#include <spall/Common/Enums/AccelerationStructureGeometryFlags.h>
#include <spall/Common/Enums/AccelerationStructureGeometryType.h>
#include <spall/Common/Enums/Format.h>
#include <spall/Common/Enums/IndexFormat.h>

#include <spall/Resources/AccelerationStructure/AccelerationStructureAabb.h>

#include <cstdint>

namespace spall
{
	class IBuffer;

	/// Describes one triangle geometry within a bottom-level acceleration structure.
	///
	/// The buffers are pinned for the lifetime of the structure. Rewriting their
	/// contents and rebuilding is supported; substituting a different buffer is not.
	struct AccelerationStructureGeometry
	{
		AccelerationStructureGeometryFlags Flags = AccelerationStructureGeometryFlags::Opaque;

		/// Selects which of the two field groups below is read. The other is ignored.
		AccelerationStructureGeometryType Type = AccelerationStructureGeometryType::Triangles;

		IBuffer* VertexBuffer = nullptr;

		/// Vertex position format. Only a subset of formats is traceable.
		Format VertexFormat = Format::RGB32Float;

		std::uint32_t VertexOffset = 0;
		std::uint32_t VertexStride = 0;
		std::uint32_t VertexCount = 0;

		/// Optional. A null index buffer builds the vertices as an unindexed triangle list.
		IBuffer* IndexBuffer = nullptr;

		IndexFormat IndexFormat = IndexFormat::UInt32;
		std::uint32_t IndexOffset = 0;
		std::uint32_t IndexCount = 0;

		/// Optional row-major 3x4 float transform applied to every vertex.
		IBuffer* TransformBuffer = nullptr;

		/// Byte offset of the transform, which must be a multiple of sixteen.
		std::uint32_t TransformOffset = 0;

		/// Array of AccelerationStructureAabb elements, read when Type is Aabbs.
		IBuffer* AabbBuffer = nullptr;

		/// Byte offset of the bounding-box array, which must be a multiple of eight.
		std::uint32_t AabbOffset = 0;

		/// Byte distance between bounding boxes, which must be a multiple of eight.
		std::uint32_t AabbStride = sizeof(float) * 6;

		std::uint32_t AabbCount = 0;
	};
} // namespace spall
