// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Pipeline/Shader/D3D12_Shader.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

#include <utility>

namespace spall::d3d12
{
	Shader::Shader(
		Device& device,
		ShaderStage stage,
		std::vector<std::byte> bytecode)
		: m_Device(&device), m_Stage(stage), m_Bytecode(std::move(bytecode))
	{
	}

	Shader::~Shader() = default;

	RenderBackendType Shader::backendType() const
	{
		return RenderBackendType::D3D12;
	}
} // namespace spall::d3d12
