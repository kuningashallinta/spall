#pragma once

#include <spall/Common/Enums/PipelineEnums.h>

#include <cstddef>
#include <span>

namespace spall
{
	/// Describes precompiled shader bytecode for one shader stage.
	struct ShaderCreateInfo
	{
		ShaderStage Stage = ShaderStage::Vertex;

		/// Bytecode consumed during createShader and not retained by the shader.
		std::span<const std::byte> Bytecode;
	};
} // namespace spall
