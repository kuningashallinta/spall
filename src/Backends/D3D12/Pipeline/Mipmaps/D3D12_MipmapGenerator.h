// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/TextureType.h>
#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace spall::d3d12
{
	class Device;

	/// Owns the blit pipeline that fills mip levels below the base level. Pipeline lookup is thread-safe.
	///
	/// D3D12 has no fixed-function downsample, so the backend carries its own
	/// pipeline. One pipeline state is cached per render-target format.
	class MipmapGenerator
	{
	public:
		Status pipelineState(
			Device& device,
			TextureType type,
			Format format,
			ID3D12RootSignature** rootSignature,
			ID3D12PipelineState** pipelineState);

	private:
		Status createRootSignature(Device& device);

		/// Volume downsampling adds a root constant naming the destination slice.
		Status createVolumeRootSignature(Device& device);

		ComPtr<ID3D12RootSignature> m_RootSignature;
		ComPtr<ID3D12RootSignature> m_VolumeRootSignature;

		/// Keyed by texture type and render-target format.
		std::unordered_map<std::uint64_t, ComPtr<ID3D12PipelineState>> m_PipelineStates;
		std::mutex m_Mutex;
	};
} // namespace spall::d3d12
