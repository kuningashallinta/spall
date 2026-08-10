// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>

#include <cstdint>
#include <vector>

namespace spall::d3d12
{
	class CommandList;
	class ComputePipeline;
	class Device;
	class GraphicsPipeline;
	class ResourceSet;
	class RootSignature;

	class ResourceSetLayout final : public SharedObject<IResourceSetLayout>
	{
	public:
		ResourceSetLayout(
			Device& device,
			std::vector<ResourceBindingInfo> bindings);

		~ResourceSetLayout(void) override;

		RenderBackendType backendType(void) const override;

		const ResourceBindingInfo* findBinding(std::uint32_t binding) const;

		/// Gets the number of registers this layout consumes in the flattened register space.
		std::uint32_t registerSpan(void) const;

		std::uint32_t viewBindingCount(void) const;
		std::uint32_t samplerBindingCount(void) const;

	private:
		Resource<Device> m_Device;
		std::vector<ResourceBindingInfo> m_Bindings;

	private:
		friend class CommandList;
		friend class ComputePipeline;
		friend class Device;
		friend class GraphicsPipeline;
		friend class ResourceSet;
		friend class RootSignature;
	};
} // namespace spall::d3d12
