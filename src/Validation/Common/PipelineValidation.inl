namespace spall
{
	inline Status validateResourceSetLayoutList(
		std::span<const IResourceSetLayout* const> resourceSetLayouts)
	{
		if (resourceSetLayouts.size() > MaxResourceSets)
		{
			return ERR_INVALID_BINDING;
		}

		for (const IResourceSetLayout* resourceSetLayout : resourceSetLayouts)
		{
			if (resourceSetLayout == nullptr)
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		return {};
	}

	inline Status validatePushConstantInfo(
		const PushConstantInfo& info,
		ShaderStageFlags supportedStages)
	{
		if ((info.Size == 0) != (info.Stages == ShaderStageFlags::None))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.Size == 0)
		{
			return {};
		}

		if (((info.Size % 4) != 0) or (info.Size > MaxPushConstantSize))
		{
			return ERR_INVALID_SIZE;
		}

		const std::uint32_t stageValue = static_cast<std::uint32_t>(info.Stages);
		const std::uint32_t supportedStageValue = static_cast<std::uint32_t>(supportedStages);

		if ((stageValue & ~supportedStageValue) != 0)
		{
			return ERR_UNSUPPORTED_SHADER_STAGE;
		}

		return {};
	}

	inline Status validateRasterizerState(
		const PipelineCreateInfo& info)
	{
		if ((info.PrimitiveTopology != PrimitiveTopology::TriangleStrip) and
			(info.PrimitiveTopology != PrimitiveTopology::TriangleList) and
			(info.PrimitiveTopology != PrimitiveTopology::LineList) and
			(info.PrimitiveTopology != PrimitiveTopology::PatchList))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.FillMode != FillMode::Solid) and (info.FillMode != FillMode::Wireframe))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.CullMode != CullMode::None) and (info.CullMode != CullMode::Front) and (info.CullMode != CullMode::Back))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.FrontFace != FrontFace::Clockwise) and (info.FrontFace != FrontFace::CounterClockwise))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((not std::isfinite(info.DepthBiasClamp)) or (not std::isfinite(info.SlopeScaledDepthBias)))
		{
			return ERR_INVALID_RANGE;
		}

		if ((not std::isfinite(info.LineWidth)) or (info.LineWidth <= 0.0f))
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateBlendState(
		const PipelineCreateInfo& info)
	{
		const std::uint32_t blendStateCount = (info.ColorTargetFormatCount < MaxColorAttachments)
			? info.ColorTargetFormatCount
			: MaxColorAttachments;

		for (std::uint32_t blendStateIndex = 0; blendStateIndex < blendStateCount; ++blendStateIndex)
		{
			const BlendStateInfo& blendState = info.BlendStates[blendStateIndex];
			const BlendFactor blendFactors[] = {
				blendState.SourceColorFactor,
				blendState.DestinationColorFactor,
				blendState.SourceAlphaFactor,
				blendState.DestinationAlphaFactor};

			for (const BlendFactor blendFactor : blendFactors)
			{
				if ((blendFactor < BlendFactor::Zero) or (blendFactor > BlendFactor::OneMinusDestinationAlpha))
				{
					return ERR_INVALID_ARGUMENT;
				}
			}

			const BlendOp blendOps[] = {blendState.ColorBlendOp, blendState.AlphaBlendOp};

			for (const BlendOp blendOp : blendOps)
			{
				if ((blendOp < BlendOp::Add) or (blendOp > BlendOp::Max))
				{
					return ERR_INVALID_ARGUMENT;
				}
			}

			if ((static_cast<std::uint32_t>(blendState.ColorWriteMask) &
					~static_cast<std::uint32_t>(ColorComponentFlags::All)) != 0)
			{
				return ERR_INVALID_ARGUMENT;
			}
		}

		return {};
	}

	inline Status validateShaderStages(
		const PipelineCreateInfo& info)
	{
		if (info.VertexShader.Module == nullptr)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		if ((info.ColorTargetFormatCount != 0) and (info.FragmentShader.Module == nullptr))
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		const bool hasTessellationControl = (info.TessellationControlShader.Module != nullptr);
		const bool hasTessellationEvaluation = (info.TessellationEvaluationShader.Module != nullptr);

		if (hasTessellationControl != hasTessellationEvaluation)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		const bool tessellated = hasTessellationControl;
		const bool patchTopology = (info.PrimitiveTopology == PrimitiveTopology::PatchList);

		if (tessellated != patchTopology)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		if (tessellated and ((info.PatchControlPoints == 0) or (info.PatchControlPoints > MaxPatchControlPoints)))
		{
			return ERR_INVALID_SIZE;
		}

		return {};
	}

	inline Status validateColorTargets(
		const PipelineCreateInfo& info)
	{
		if (info.ColorTargetFormatCount > MaxColorAttachments)
		{
			return ERR_INVALID_FORMAT;
		}

		if ((info.ColorTargetFormatCount == 0) and (info.DepthStencilFormat == Format::Unknown))
		{
			return ERR_INVALID_FORMAT;
		}

		if (not isValidSampleCount(info.SampleCount))
		{
			return ERR_INVALID_SIZE;
		}

		for (std::uint32_t colorIndex = 0; colorIndex < info.ColorTargetFormatCount; ++colorIndex)
		{
			if (not isRenderTargetFormat(info.ColorTargetFormats[colorIndex]))
			{
				return ERR_INVALID_FORMAT;
			}
		}

		return {};
	}

	inline Status validateDepthState(
		const PipelineCreateInfo& info)
	{
		if ((info.DepthCompareOp < CompareOp::Never) or (info.DepthCompareOp > CompareOp::Always))
		{
			return ERR_INVALID_ARGUMENT;
		}

		if ((info.DepthStencilFormat != Format::Unknown) and (not isDepthFormat(info.DepthStencilFormat)))
		{
			return ERR_INVALID_FORMAT;
		}

		if ((info.EnableDepthTest or info.EnableDepthWrite) and (info.DepthStencilFormat == Format::Unknown))
		{
			return ERR_INVALID_FORMAT;
		}

		if (info.EnableDepthWrite and (not info.EnableDepthTest))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (info.EnableStencilTest and (not hasStencilAspect(info.DepthStencilFormat)))
		{
			return ERR_INVALID_FORMAT;
		}

		const StencilFaceStateInfo stencilStates[] = {info.FrontStencilState, info.BackStencilState};

		for (const StencilFaceStateInfo& stencilState : stencilStates)
		{
			if ((stencilState.Compare < CompareOp::Never) or (stencilState.Compare > CompareOp::Always))
			{
				return ERR_INVALID_ARGUMENT;
			}

			const StencilOp stencilOps[] = {stencilState.FailOp, stencilState.DepthFailOp, stencilState.PassOp};

			for (const StencilOp stencilOp : stencilOps)
			{
				if ((stencilOp < StencilOp::Keep) or (stencilOp > StencilOp::DecrementWrap))
				{
					return ERR_INVALID_ARGUMENT;
				}
			}
		}

		return {};
	}

	inline Status validateVertexLayout(
		const PipelineCreateInfo& info)
	{
		if (info.VertexBindings.empty() != info.VertexAttributes.empty())
		{
			return ERR_INVALID_BINDING;
		}

		for (std::size_t bindingIndex = 0; bindingIndex < info.VertexBindings.size(); ++bindingIndex)
		{
			if (info.VertexBindings[bindingIndex].Stride == 0)
			{
				return ERR_INVALID_BINDING;
			}

			for (std::size_t compareIndex = bindingIndex + 1; compareIndex < info.VertexBindings.size(); ++compareIndex)
			{
				if (info.VertexBindings[bindingIndex].Binding == info.VertexBindings[compareIndex].Binding)
				{
					return ERR_INVALID_BINDING;
				}
			}
		}

		for (std::size_t attributeIndex = 0; attributeIndex < info.VertexAttributes.size(); ++attributeIndex)
		{
			const VertexAttributeInfo& attribute = info.VertexAttributes[attributeIndex];

			if (not isVertexFormat(attribute.Format))
			{
				return ERR_INVALID_FORMAT;
			}

			const VertexBindingInfo* matchingBinding = nullptr;

			for (std::size_t bindingIndex = 0; bindingIndex < info.VertexBindings.size(); ++bindingIndex)
			{
				if (info.VertexBindings[bindingIndex].Binding == attribute.Binding)
				{
					matchingBinding = &info.VertexBindings[bindingIndex];
					break;
				}
			}

			if (matchingBinding == nullptr)
			{
				return ERR_INVALID_BINDING;
			}

			const std::uint64_t attributeEnd = static_cast<std::uint64_t>(attribute.Offset) + static_cast<std::uint64_t>(formatBytesPerPixel(attribute.Format));

			if (attributeEnd > matchingBinding->Stride)
			{
				return ERR_INVALID_BINDING;
			}

			for (std::size_t compareIndex = attributeIndex + 1; compareIndex < info.VertexAttributes.size(); ++compareIndex)
			{
				if (attribute.Location == info.VertexAttributes[compareIndex].Location)
				{
					return ERR_INVALID_BINDING;
				}
			}
		}

		return {};
	}

	inline Status validatePipelineCreateInfo(
		const PipelineCreateInfo& info)
	{
		ShaderStageFlags pipelineStages = ShaderStageFlags::Vertex;

		if (info.FragmentShader.Module != nullptr)
		{
			pipelineStages |= ShaderStageFlags::Fragment;
		}

		if (info.GeometryShader.Module != nullptr)
		{
			pipelineStages |= ShaderStageFlags::Geometry;
		}

		if (info.TessellationControlShader.Module != nullptr)
		{
			pipelineStages |= ShaderStageFlags::TessellationControl;
		}

		if (info.TessellationEvaluationShader.Module != nullptr)
		{
			pipelineStages |= ShaderStageFlags::TessellationEvaluation;
		}

		SPALL_TRY(validateRasterizerState(info));
		SPALL_TRY(validateBlendState(info));
		SPALL_TRY(validateDepthState(info));
		SPALL_TRY(validateShaderStages(info));
		SPALL_TRY(validateColorTargets(info));
		SPALL_TRY(validateResourceSetLayoutList(info.ResourceSetLayouts));
		SPALL_TRY(validatePushConstantInfo(info.PushConstants, pipelineStages));
		SPALL_TRY(validateVertexLayout(info));

		return {};
	}

	inline Status validateComputePipelineCreateInfo(
		const ComputePipelineCreateInfo& info)
	{
		if (info.ComputeShader.Module == nullptr)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		SPALL_TRY(validateResourceSetLayoutList(info.ResourceSetLayouts));
		SPALL_TRY(validatePushConstantInfo(info.PushConstants, ShaderStageFlags::Compute));

		return {};
	}

	inline Status validateRayTracingShaderStage(
		const PipelineShaderStageInfo& stage)
	{
		if ((stage.Entry == nullptr) or (stage.Entry[0] == '\0'))
		{
			return ERR_INVALID_ARGUMENT;
		}

		return {};
	}

	inline Status validateRayTracingPipelineCreateInfo(
		const RayTracingPipelineCreateInfo& info)
	{
		if (info.RayGenerationShader.Module == nullptr)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		SPALL_TRY(validateRayTracingShaderStage(info.RayGenerationShader));

		for (const PipelineShaderStageInfo& missShader : info.MissShaders)
		{
			if (missShader.Module == nullptr)
			{
				return ERR_INVALID_SHADER_STAGE;
			}

			SPALL_TRY(validateRayTracingShaderStage(missShader));
		}

		bool triangleGroup = false;

		for (const RayTracingHitGroup& hitGroup : info.HitGroups)
		{
			if ((hitGroup.ClosestHitShader.Module == nullptr) and
				(hitGroup.AnyHitShader.Module == nullptr) and
				(hitGroup.IntersectionShader.Module == nullptr))
			{
				return ERR_INVALID_SHADER_STAGE;
			}

			if (hitGroup.ClosestHitShader.Module != nullptr)
			{
				SPALL_TRY(validateRayTracingShaderStage(hitGroup.ClosestHitShader));
			}

			if (hitGroup.AnyHitShader.Module != nullptr)
			{
				SPALL_TRY(validateRayTracingShaderStage(hitGroup.AnyHitShader));
			}

			if (hitGroup.IntersectionShader.Module != nullptr)
			{
				SPALL_TRY(validateRayTracingShaderStage(hitGroup.IntersectionShader));
			}
			else
			{
				triangleGroup = true;
			}
		}

		if ((info.MaxPayloadSize == 0) or ((info.MaxPayloadSize % 4) != 0))
		{
			return ERR_INVALID_SIZE;
		}

		if ((info.MaxAttributeSize < 4) or (info.MaxAttributeSize > MaxRayTracingAttributeSize) or
			((info.MaxAttributeSize % 4) != 0))
		{
			return ERR_INVALID_SIZE;
		}

		if (triangleGroup and (info.MaxAttributeSize < 8))
		{
			return ERR_INVALID_SIZE;
		}

		if ((info.MaxRecursionDepth == 0) or (info.MaxRecursionDepth > MaxRayRecursionDepth))
		{
			return ERR_INVALID_RANGE;
		}

		constexpr ShaderStageFlags rayTracingStages = ShaderStageFlags::RayGeneration | ShaderStageFlags::Miss |
			ShaderStageFlags::ClosestHit | ShaderStageFlags::AnyHit | ShaderStageFlags::Intersection;

		SPALL_TRY(validateResourceSetLayoutList(info.ResourceSetLayouts));
		SPALL_TRY(validatePushConstantInfo(info.PushConstants, rayTracingStages));

		return {};
	}
} // namespace spall
