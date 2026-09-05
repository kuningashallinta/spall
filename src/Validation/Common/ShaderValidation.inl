// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Status validateShaderCreateInfo(
		const ShaderCreateInfo& info)
	{
		if (info.Bytecode.empty())
		{
			return ERR_INVALID_SHADER_BYTECODE;
		}

		switch (info.Stage)
		{
			case ShaderStage::Vertex:
			case ShaderStage::Fragment:
			case ShaderStage::Compute:
			case ShaderStage::Geometry:
			case ShaderStage::TessellationControl:
			case ShaderStage::TessellationEvaluation:
			case ShaderStage::RayGeneration:
			case ShaderStage::Miss:
			case ShaderStage::ClosestHit:
			case ShaderStage::AnyHit:
			case ShaderStage::Intersection:
			{
				break;
			}

			default:
			{
				return ERR_UNSUPPORTED_SHADER_STAGE;
			}
		}

		return {};
	}
} // namespace spall
