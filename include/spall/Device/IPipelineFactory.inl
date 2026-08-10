// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Resource<IShader> IPipelineFactory::createShader(
		const ShaderCreateInfo& info)
	{
		Resource<IShader> shader;
		createShader(info, &shader);
		return shader;
	}

	template <typename T, std::size_t Extent>
		requires std::is_trivially_copyable_v<T>
	inline Resource<IShader> IPipelineFactory::createShader(
		ShaderStage stage,
		std::span<const T, Extent> bytecode)
	{
		return createShader({stage, std::as_bytes(bytecode)});
	}

	template <typename T, std::size_t Size>
		requires std::is_trivially_copyable_v<T>
	inline Resource<IShader> IPipelineFactory::createShader(
		ShaderStage stage,
		const T (&bytecode)[Size])
	{
		return createShader(stage, std::span {bytecode});
	}

	inline Resource<IResourceSetLayout> IPipelineFactory::createResourceSetLayout(
		const ResourceSetLayoutCreateInfo& info)
	{
		Resource<IResourceSetLayout> resourceSetLayout;
		createResourceSetLayout(info, &resourceSetLayout);
		return resourceSetLayout;
	}

	inline Resource<IResourceSet> IPipelineFactory::createResourceSet(
		const ResourceSetCreateInfo& info)
	{
		Resource<IResourceSet> resourceSet;
		createResourceSet(info, &resourceSet);
		return resourceSet;
	}

	inline Resource<IPipeline> IPipelineFactory::createPipeline(
		const PipelineCreateInfo& info)
	{
		Resource<IPipeline> pipeline;
		createPipeline(info, &pipeline);
		return pipeline;
	}

	inline Resource<IPipeline> IPipelineFactory::createComputePipeline(
		const ComputePipelineCreateInfo& info)
	{
		Resource<IPipeline> pipeline;
		createComputePipeline(info, &pipeline);
		return pipeline;
	}

	inline Resource<IPipeline> IPipelineFactory::createRayTracingPipeline(
		const RayTracingPipelineCreateInfo& info)
	{
		Resource<IPipeline> pipeline;
		createRayTracingPipeline(info, &pipeline);
		return pipeline;
	}
} // namespace spall
