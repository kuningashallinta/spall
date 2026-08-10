// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <string>
#include <vector>

namespace spall::d3d12
{
	class Buffer;
	class CommandList;
	class Device;
	class ResourceSet;

	/// Lives permanently in the raytracing-acceleration-structure state, which
	/// D3D12 never allows a resource to leave, so it stays outside the command
	/// list's state tracker and is ordered by unordered-access barriers instead.
	class AccelerationStructure final : public SharedObject<IAccelerationStructure>
	{
	public:
		AccelerationStructure(
			Device& device,
			const AccelerationStructureInfo& info,
			ComPtr<ID3D12Resource> resource,
			std::vector<Resource<Buffer>> inputBuffers,
			std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescriptions,
			Resource<Buffer> instanceBuffer,
			std::uint32_t instanceBufferOffset);

		~AccelerationStructure(void) override;

		RenderBackendType backendType(void) const override;
		AccelerationStructureInfo info(void) const override;
		std::uint64_t deviceAddress(void) const override;

	private:
		Resource<Device> m_Device;

		std::string m_DebugName;
		AccelerationStructureInfo m_Info = {};

		ComPtr<ID3D12Resource> m_Resource;

		/// Vertex, index, and transform buffers a recorded build reads, kept
		/// alive and transitioned on its behalf.
		std::vector<Resource<Buffer>> m_InputBuffers;

		/// Native geometry resolved once at creation, so prebuild sizing and
		/// every later build see the identical description.
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_GeometryDescriptions;

		Resource<Buffer> m_InstanceBuffer;
		std::uint32_t m_InstanceBufferOffset = 0;

		/// Set by the first recorded build, which an update requires.
		///
		/// Recording is what sets this, not execution, so a build that is
		/// recorded and then never submitted still makes a later update legal.
		bool m_Built = false;

		/// Instance count the most recent recorded build used. An update has to
		/// refit exactly that many, which D3D12 does not diagnose.
		std::uint32_t m_BuiltInstanceCount = 0;

		/// Postbuild-info destination and its readback copy, present only with
		/// AllowCompaction. Every build measures into them.
		ComPtr<ID3D12Resource> m_CompactedSizeBuffer;
		ComPtr<ID3D12Resource> m_CompactedSizeReadback;

		bool m_Compacted = false;

	private:
		friend class CommandList;
		friend class Device;
		friend class ResourceSet;
	};
} // namespace spall::d3d12
