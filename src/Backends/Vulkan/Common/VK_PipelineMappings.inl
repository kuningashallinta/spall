namespace spall::vk
{
	inline VkPrimitiveTopology vulkanPrimitiveTopology(
		PrimitiveTopology topology)
	{
		switch (topology)
		{
			case PrimitiveTopology::TriangleList:
			{
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			}

			case PrimitiveTopology::LineList:
			{
				return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			}

			case PrimitiveTopology::PatchList:
			{
				return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}

			case PrimitiveTopology::TriangleStrip:
			default:
			{
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			}
		}
	}

	inline VkCullModeFlags vulkanCullMode(
		CullMode cullMode)
	{
		switch (cullMode)
		{
			case CullMode::Front:
			{
				return VK_CULL_MODE_FRONT_BIT;
			}

			case CullMode::Back:
			{
				return VK_CULL_MODE_BACK_BIT;
			}

			case CullMode::None:
			default:
			{
				return VK_CULL_MODE_NONE;
			}
		}
	}

	inline VkPolygonMode vulkanPolygonMode(
		FillMode fillMode)
	{
		switch (fillMode)
		{
			case FillMode::Wireframe:
			{
				return VK_POLYGON_MODE_LINE;
			}

			case FillMode::Solid:
			default:
			{
				return VK_POLYGON_MODE_FILL;
			}
		}
	}

	inline VkFrontFace vulkanFrontFace(
		FrontFace frontFace)
	{
		switch (frontFace)
		{
			case FrontFace::CounterClockwise:
			{
				return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			}

			case FrontFace::Clockwise:
			default:
			{
				return VK_FRONT_FACE_CLOCKWISE;
			}
		}
	}

	inline VkBlendFactor vulkanBlendFactor(
		BlendFactor blendFactor)
	{
		switch (blendFactor)
		{
			case BlendFactor::Zero:
			{
				return VK_BLEND_FACTOR_ZERO;
			}

			case BlendFactor::SourceColor:
			{
				return VK_BLEND_FACTOR_SRC_COLOR;
			}

			case BlendFactor::OneMinusSourceColor:
			{
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			}

			case BlendFactor::DestinationColor:
			{
				return VK_BLEND_FACTOR_DST_COLOR;
			}

			case BlendFactor::OneMinusDestinationColor:
			{
				return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			}

			case BlendFactor::SourceAlpha:
			{
				return VK_BLEND_FACTOR_SRC_ALPHA;
			}

			case BlendFactor::OneMinusSourceAlpha:
			{
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			}

			case BlendFactor::DestinationAlpha:
			{
				return VK_BLEND_FACTOR_DST_ALPHA;
			}

			case BlendFactor::OneMinusDestinationAlpha:
			{
				return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			}

			case BlendFactor::One:
			default:
			{
				return VK_BLEND_FACTOR_ONE;
			}
		}
	}

	inline VkCompareOp vulkanCompareOp(
		CompareOp compareOp)
	{
		switch (compareOp)
		{
			case CompareOp::Never:
			{
				return VK_COMPARE_OP_NEVER;
			}

			case CompareOp::Less:
			{
				return VK_COMPARE_OP_LESS;
			}

			case CompareOp::Equal:
			{
				return VK_COMPARE_OP_EQUAL;
			}

			case CompareOp::Greater:
			{
				return VK_COMPARE_OP_GREATER;
			}

			case CompareOp::NotEqual:
			{
				return VK_COMPARE_OP_NOT_EQUAL;
			}

			case CompareOp::GreaterOrEqual:
			{
				return VK_COMPARE_OP_GREATER_OR_EQUAL;
			}

			case CompareOp::Always:
			{
				return VK_COMPARE_OP_ALWAYS;
			}

			case CompareOp::LessOrEqual:
			default:
			{
				return VK_COMPARE_OP_LESS_OR_EQUAL;
			}
		}
	}

	inline VkStencilOp vulkanStencilOp(
		StencilOp stencilOp)
	{
		switch (stencilOp)
		{
			case StencilOp::Zero:
			{
				return VK_STENCIL_OP_ZERO;
			}

			case StencilOp::Replace:
			{
				return VK_STENCIL_OP_REPLACE;
			}

			case StencilOp::IncrementClamp:
			{
				return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			}

			case StencilOp::DecrementClamp:
			{
				return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			}

			case StencilOp::Invert:
			{
				return VK_STENCIL_OP_INVERT;
			}

			case StencilOp::IncrementWrap:
			{
				return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			}

			case StencilOp::DecrementWrap:
			{
				return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			}

			case StencilOp::Keep:
			default:
			{
				return VK_STENCIL_OP_KEEP;
			}
		}
	}

	inline VkColorComponentFlags vulkanColorComponentFlags(
		ColorComponentFlags mask)
	{
		VkColorComponentFlags componentFlags = 0;

		if ((mask & ColorComponentFlags::Red) != ColorComponentFlags::None)
		{
			componentFlags |= VK_COLOR_COMPONENT_R_BIT;
		}

		if ((mask & ColorComponentFlags::Green) != ColorComponentFlags::None)
		{
			componentFlags |= VK_COLOR_COMPONENT_G_BIT;
		}

		if ((mask & ColorComponentFlags::Blue) != ColorComponentFlags::None)
		{
			componentFlags |= VK_COLOR_COMPONENT_B_BIT;
		}

		if ((mask & ColorComponentFlags::Alpha) != ColorComponentFlags::None)
		{
			componentFlags |= VK_COLOR_COMPONENT_A_BIT;
		}

		return componentFlags;
	}

	inline VkBlendOp vulkanBlendOp(
		BlendOp blendOp)
	{
		switch (blendOp)
		{
			case BlendOp::Subtract:
			{
				return VK_BLEND_OP_SUBTRACT;
			}

			case BlendOp::ReverseSubtract:
			{
				return VK_BLEND_OP_REVERSE_SUBTRACT;
			}

			case BlendOp::Min:
			{
				return VK_BLEND_OP_MIN;
			}

			case BlendOp::Max:
			{
				return VK_BLEND_OP_MAX;
			}

			case BlendOp::Add:
			default:
			{
				return VK_BLEND_OP_ADD;
			}
		}
	}

	inline VkPipelineColorBlendAttachmentState vulkanColorBlendAttachmentState(
		const BlendStateInfo& blendState)
	{
		VkPipelineColorBlendAttachmentState state = {};
		state.blendEnable = blendState.EnableBlend ? VK_TRUE : VK_FALSE;
		state.srcColorBlendFactor = vulkanBlendFactor(blendState.SourceColorFactor);
		state.dstColorBlendFactor = vulkanBlendFactor(blendState.DestinationColorFactor);
		state.colorBlendOp = vulkanBlendOp(blendState.ColorBlendOp);
		state.srcAlphaBlendFactor = vulkanBlendFactor(blendState.SourceAlphaFactor);
		state.dstAlphaBlendFactor = vulkanBlendFactor(blendState.DestinationAlphaFactor);
		state.alphaBlendOp = vulkanBlendOp(blendState.AlphaBlendOp);
		state.colorWriteMask = vulkanColorComponentFlags(blendState.ColorWriteMask);

		return state;
	}
} // namespace spall::vk
