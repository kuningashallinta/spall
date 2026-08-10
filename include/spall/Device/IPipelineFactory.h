// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Binding/ResourceSetCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceSetLayoutCreateInfo.h>
#include <spall/Pipeline/Pipeline/ComputePipelineCreateInfo.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Pipeline/PipelineCreateInfo.h>
#include <spall/Pipeline/Pipeline/RayTracingPipelineCreateInfo.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Pipeline/Shader/ShaderCreateInfo.h>

#include <cstddef>
#include <span>
#include <type_traits>

namespace spall
{
	/// Creates shaders, binding objects, and pipelines owned by a graphics device.
	///
	/// Convenience overloads return an empty resource on failure; Status
	/// overloads preserve detailed failure information.
	class IPipelineFactory
	{
	public:
		virtual ~IPipelineFactory(void) = default;

		Resource<IShader> createShader(
			const ShaderCreateInfo& info);

		template <typename T, std::size_t Extent>
			requires std::is_trivially_copyable_v<T>
		Resource<IShader> createShader(
			ShaderStage stage,
			std::span<const T, Extent> bytecode);

		template <typename T, std::size_t Size>
			requires std::is_trivially_copyable_v<T>
		Resource<IShader> createShader(
			ShaderStage stage,
			const T (&bytecode)[Size]);

		virtual Status createShader(
			const ShaderCreateInfo& info,
			Resource<IShader>* shader) = 0;

		Resource<IResourceSetLayout> createResourceSetLayout(
			const ResourceSetLayoutCreateInfo& info);

		virtual Status createResourceSetLayout(
			const ResourceSetLayoutCreateInfo& info,
			Resource<IResourceSetLayout>* resourceSetLayout) = 0;

		Resource<IResourceSet> createResourceSet(
			const ResourceSetCreateInfo& info);

		virtual Status createResourceSet(
			const ResourceSetCreateInfo& info,
			Resource<IResourceSet>* resourceSet) = 0;

		Resource<IPipeline> createPipeline(
			const PipelineCreateInfo& info);

		virtual Status createPipeline(
			const PipelineCreateInfo& info,
			Resource<IPipeline>* pipeline) = 0;

		Resource<IPipeline> createComputePipeline(
			const ComputePipelineCreateInfo& info);

		virtual Status createComputePipeline(
			const ComputePipelineCreateInfo& info,
			Resource<IPipeline>* pipeline) = 0;

		Resource<IPipeline> createRayTracingPipeline(
			const RayTracingPipelineCreateInfo& info);

		virtual Status createRayTracingPipeline(
			const RayTracingPipelineCreateInfo& info,
			Resource<IPipeline>* pipeline) = 0;
	};
} // namespace spall

#include <spall/Device/IPipelineFactory.inl>
