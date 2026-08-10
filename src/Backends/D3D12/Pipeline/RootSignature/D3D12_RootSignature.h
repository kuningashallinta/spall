// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>

#include <spall/Common/Enums/PipelineType.h>
#include <spall/Common/Enums/ShaderStageFlags.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/PushConstantInfo.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace spall::d3d12
{
	/// Register reserved for the portable push-constant block.
	inline constexpr std::uint32_t PushConstantRegister = 13;

	class Device;
	class ResourceSetLayout;

	/// Maps resource-set layouts onto root parameters using the portable flattened register space.
	class RootSignature
	{
	public:
		struct SetTables
		{
			std::uint32_t ViewParameterIndex = InvalidRootParameter;
			std::uint32_t SamplerParameterIndex = InvalidRootParameter;
			std::uint32_t ViewDescriptorCount = 0;
			std::uint32_t SamplerDescriptorCount = 0;
			std::uint32_t RegisterOffset = 0;
		};

		static Status create(
			Device& device,
			std::span<const IResourceSetLayout* const> layouts,
			const PushConstantInfo& pushConstants,
			PipelineType type,
			std::unique_ptr<RootSignature>* rootSignature);

		~RootSignature(void);

		std::uint32_t setCount(void) const;
		const SetTables& setTables(std::uint32_t slot) const;
		const ResourceSetLayout* layout(std::uint32_t slot) const;

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature;

		std::vector<Resource<ResourceSetLayout>> m_Layouts;
		std::vector<SetTables> m_SetTables;

		std::uint32_t m_PushConstantParameterIndex = InvalidRootParameter;
		std::uint32_t m_PushConstantSize = 0;
		ShaderStageFlags m_PushConstantStages = ShaderStageFlags::None;

	private:
		friend class CommandList;
		friend class ComputePipeline;
		friend class Device;
		friend class GraphicsPipeline;
	};
} // namespace spall::d3d12
