namespace spall::d3d12
{
	inline D3D12_PRIMITIVE_TOPOLOGY primitiveTopology(
		PrimitiveTopology topology,
		std::uint32_t patchControlPoints)
	{
		switch (topology)
		{
			case PrimitiveTopology::TriangleList:
			{
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			}

			case PrimitiveTopology::PatchList:
			{
				return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(
					D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (patchControlPoints - 1));
			}

			case PrimitiveTopology::TriangleStrip:
			default:
			{
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			}
		}
	}

	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType(
		PrimitiveTopology topology)
	{
		if (topology == PrimitiveTopology::PatchList)
		{
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		}

		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	inline D3D12_CULL_MODE cullMode(
		CullMode mode)
	{
		switch (mode)
		{
			case CullMode::Front:
			{
				return D3D12_CULL_MODE_FRONT;
			}

			case CullMode::Back:
			{
				return D3D12_CULL_MODE_BACK;
			}

			case CullMode::None:
			default:
			{
				return D3D12_CULL_MODE_NONE;
			}
		}
	}

	inline D3D12_FILL_MODE fillMode(
		FillMode mode)
	{
		switch (mode)
		{
			case FillMode::Wireframe:
			{
				return D3D12_FILL_MODE_WIREFRAME;
			}

			case FillMode::Solid:
			default:
			{
				return D3D12_FILL_MODE_SOLID;
			}
		}
	}

	inline D3D12_BLEND blendFactor(
		BlendFactor factor)
	{
		switch (factor)
		{
			case BlendFactor::Zero:
			{
				return D3D12_BLEND_ZERO;
			}

			case BlendFactor::SourceColor:
			{
				return D3D12_BLEND_SRC_COLOR;
			}

			case BlendFactor::OneMinusSourceColor:
			{
				return D3D12_BLEND_INV_SRC_COLOR;
			}

			case BlendFactor::DestinationColor:
			{
				return D3D12_BLEND_DEST_COLOR;
			}

			case BlendFactor::OneMinusDestinationColor:
			{
				return D3D12_BLEND_INV_DEST_COLOR;
			}

			case BlendFactor::SourceAlpha:
			{
				return D3D12_BLEND_SRC_ALPHA;
			}

			case BlendFactor::OneMinusSourceAlpha:
			{
				return D3D12_BLEND_INV_SRC_ALPHA;
			}

			case BlendFactor::DestinationAlpha:
			{
				return D3D12_BLEND_DEST_ALPHA;
			}

			case BlendFactor::OneMinusDestinationAlpha:
			{
				return D3D12_BLEND_INV_DEST_ALPHA;
			}

			case BlendFactor::One:
			default:
			{
				return D3D12_BLEND_ONE;
			}
		}
	}

	inline D3D12_BLEND_OP blendOp(
		BlendOp op)
	{
		switch (op)
		{
			case BlendOp::Subtract:
			{
				return D3D12_BLEND_OP_SUBTRACT;
			}

			case BlendOp::ReverseSubtract:
			{
				return D3D12_BLEND_OP_REV_SUBTRACT;
			}

			case BlendOp::Min:
			{
				return D3D12_BLEND_OP_MIN;
			}

			case BlendOp::Max:
			{
				return D3D12_BLEND_OP_MAX;
			}

			case BlendOp::Add:
			default:
			{
				return D3D12_BLEND_OP_ADD;
			}
		}
	}

	inline D3D12_COMPARISON_FUNC compareOp(
		CompareOp op)
	{
		switch (op)
		{
			case CompareOp::Never:
			{
				return D3D12_COMPARISON_FUNC_NEVER;
			}

			case CompareOp::Less:
			{
				return D3D12_COMPARISON_FUNC_LESS;
			}

			case CompareOp::Equal:
			{
				return D3D12_COMPARISON_FUNC_EQUAL;
			}

			case CompareOp::Greater:
			{
				return D3D12_COMPARISON_FUNC_GREATER;
			}

			case CompareOp::NotEqual:
			{
				return D3D12_COMPARISON_FUNC_NOT_EQUAL;
			}

			case CompareOp::GreaterOrEqual:
			{
				return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			}

			case CompareOp::Always:
			{
				return D3D12_COMPARISON_FUNC_ALWAYS;
			}

			case CompareOp::LessOrEqual:
			default:
			{
				return D3D12_COMPARISON_FUNC_LESS_EQUAL;
			}
		}
	}

	inline D3D12_STENCIL_OP stencilOp(
		StencilOp op)
	{
		switch (op)
		{
			case StencilOp::Zero:
			{
				return D3D12_STENCIL_OP_ZERO;
			}

			case StencilOp::Replace:
			{
				return D3D12_STENCIL_OP_REPLACE;
			}

			case StencilOp::IncrementClamp:
			{
				return D3D12_STENCIL_OP_INCR_SAT;
			}

			case StencilOp::DecrementClamp:
			{
				return D3D12_STENCIL_OP_DECR_SAT;
			}

			case StencilOp::Invert:
			{
				return D3D12_STENCIL_OP_INVERT;
			}

			case StencilOp::IncrementWrap:
			{
				return D3D12_STENCIL_OP_INCR;
			}

			case StencilOp::DecrementWrap:
			{
				return D3D12_STENCIL_OP_DECR;
			}

			case StencilOp::Keep:
			default:
			{
				return D3D12_STENCIL_OP_KEEP;
			}
		}
	}

	inline UINT8 colorWriteMask(
		ColorComponentFlags mask)
	{
		UINT8 writeMask = 0;

		if ((mask & ColorComponentFlags::Red) != ColorComponentFlags::None)
		{
			writeMask |= D3D12_COLOR_WRITE_ENABLE_RED;
		}

		if ((mask & ColorComponentFlags::Green) != ColorComponentFlags::None)
		{
			writeMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
		}

		if ((mask & ColorComponentFlags::Blue) != ColorComponentFlags::None)
		{
			writeMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
		}

		if ((mask & ColorComponentFlags::Alpha) != ColorComponentFlags::None)
		{
			writeMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		}

		return writeMask;
	}

	inline D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDescription(
		const BlendStateInfo& state)
	{
		D3D12_RENDER_TARGET_BLEND_DESC description = {};
		description.BlendEnable = state.EnableBlend ? TRUE : FALSE;
		description.LogicOpEnable = FALSE;
		description.SrcBlend = blendFactor(state.SourceColorFactor);
		description.DestBlend = blendFactor(state.DestinationColorFactor);
		description.BlendOp = blendOp(state.ColorBlendOp);
		description.SrcBlendAlpha = blendFactor(state.SourceAlphaFactor);
		description.DestBlendAlpha = blendFactor(state.DestinationAlphaFactor);
		description.BlendOpAlpha = blendOp(state.AlphaBlendOp);
		description.LogicOp = D3D12_LOGIC_OP_NOOP;
		description.RenderTargetWriteMask = colorWriteMask(state.ColorWriteMask);

		return description;
	}

	inline D3D12_DEPTH_STENCILOP_DESC stencilFaceDescription(
		const StencilFaceStateInfo& state)
	{
		D3D12_DEPTH_STENCILOP_DESC description = {};
		description.StencilFailOp = stencilOp(state.FailOp);
		description.StencilDepthFailOp = stencilOp(state.DepthFailOp);
		description.StencilPassOp = stencilOp(state.PassOp);
		description.StencilFunc = compareOp(state.Compare);

		return description;
	}

	inline D3D12_SHADER_VISIBILITY shaderVisibility(
		ShaderStageFlags stages)
	{
		switch (stages)
		{
			case ShaderStageFlags::Vertex:
			{
				return D3D12_SHADER_VISIBILITY_VERTEX;
			}

			case ShaderStageFlags::Fragment:
			{
				return D3D12_SHADER_VISIBILITY_PIXEL;
			}

			case ShaderStageFlags::Geometry:
			{
				return D3D12_SHADER_VISIBILITY_GEOMETRY;
			}

			case ShaderStageFlags::TessellationControl:
			{
				return D3D12_SHADER_VISIBILITY_HULL;
			}

			case ShaderStageFlags::TessellationEvaluation:
			{
				return D3D12_SHADER_VISIBILITY_DOMAIN;
			}

			default:
			{
				return D3D12_SHADER_VISIBILITY_ALL;
			}
		}
	}
} // namespace spall::d3d12
