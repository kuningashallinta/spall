#pragma once

namespace spall
{
	class IShader;

	struct PipelineShaderStageInfo
	{
		IShader* Module = nullptr;
		const char* Entry = nullptr;
	};
} // namespace spall
