// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Pipeline/Shader/IShader.h>

#include <cstddef>
#include <vector>

namespace spall::d3d12
{
	class ComputePipeline;
	class Device;
	class GraphicsPipeline;

	class Shader final : public SharedObject<IShader>
	{
	public:
		Shader(
			Device& device,
			ShaderStage stage,
			std::vector<std::byte> bytecode);

		~Shader(void) override;

		RenderBackendType backendType(void) const override;

	private:
		Resource<Device> m_Device;

		ShaderStage m_Stage = ShaderStage::Vertex;
		std::vector<std::byte> m_Bytecode;

	private:
		friend class ComputePipeline;
		friend class Device;
		friend class GraphicsPipeline;
	};
} // namespace spall::d3d12
