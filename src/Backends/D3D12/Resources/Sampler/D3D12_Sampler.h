// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/Sampler/ISampler.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>

namespace spall::d3d12
{
	class Device;
	class ResourceSet;

	class Sampler final : public SharedObject<ISampler>
	{
	public:
		Sampler(
			Device& device,
			std::uint32_t descriptorIndex);

		~Sampler(void) override;

		RenderBackendType backendType(void) const override;

	private:
		Resource<Device> m_Device;

		std::uint32_t m_DescriptorIndex = InvalidDescriptorIndex;

	private:
		friend class CommandList;
		friend class Device;
		friend class ResourceSet;
	};
} // namespace spall::d3d12
