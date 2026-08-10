// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Resources/AccelerationStructure/D3D12_AccelerationStructure.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Resources/Buffer/D3D12_Buffer.h>

#include <utility>

namespace spall::d3d12
{
	AccelerationStructure::AccelerationStructure(
		Device& device,
		const AccelerationStructureInfo& info,
		ComPtr<ID3D12Resource> resource,
		std::vector<Resource<Buffer>> inputBuffers,
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescriptions,
		Resource<Buffer> instanceBuffer,
		std::uint32_t instanceBufferOffset)
		: m_Device(&device), m_Info(info), m_Resource(std::move(resource)), m_InputBuffers(std::move(inputBuffers)), m_GeometryDescriptions(std::move(geometryDescriptions)), m_InstanceBuffer(std::move(instanceBuffer)), m_InstanceBufferOffset(instanceBufferOffset)
	{
		if (info.DebugName != nullptr)
		{
			m_DebugName = info.DebugName;
			m_Info.DebugName = m_DebugName.c_str();
		}
		else
		{
			m_Info.DebugName = nullptr;
		}
	}

	AccelerationStructure::~AccelerationStructure() = default;

	RenderBackendType AccelerationStructure::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	AccelerationStructureInfo AccelerationStructure::info() const
	{
		return m_Info;
	}

	std::uint64_t AccelerationStructure::deviceAddress() const
	{
		return static_cast<std::uint64_t>(m_Resource->GetGPUVirtualAddress());
	}
} // namespace spall::d3d12
