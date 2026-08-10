// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSetLayout.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	ResourceSetLayout::ResourceSetLayout(
		Device& device,
		std::vector<ResourceBindingInfo> bindings)
		: m_Device(&device), m_Bindings(std::move(bindings))
	{
	}

	ResourceSetLayout::~ResourceSetLayout() = default;

	RenderBackendType ResourceSetLayout::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	const ResourceBindingInfo* ResourceSetLayout::findBinding(
		std::uint32_t binding) const
	{
		for (const ResourceBindingInfo& bindingInfo : m_Bindings)
		{
			if (bindingInfo.Binding == binding)
			{
				return &bindingInfo;
			}
		}

		return nullptr;
	}

	std::uint32_t ResourceSetLayout::registerSpan() const
	{
		std::uint32_t span = 0;

		for (const ResourceBindingInfo& bindingInfo : m_Bindings)
		{
			if (span < (bindingInfo.Binding + 1))
			{
				span = bindingInfo.Binding + 1;
			}
		}

		return span;
	}

	std::uint32_t ResourceSetLayout::viewBindingCount() const
	{
		return static_cast<std::uint32_t>(m_Bindings.size());
	}

	std::uint32_t ResourceSetLayout::samplerBindingCount() const
	{
		std::uint32_t count = 0;

		for (const ResourceBindingInfo& bindingInfo : m_Bindings)
		{
			if (bindingInfo.Type == ResourceBindingType::SampledTexture)
			{
				++count;
			}
		}

		return count;
	}
} // namespace spall::d3d12
