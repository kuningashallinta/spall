namespace spall::d3d12
{
	inline D3D12_RESOURCE_STATES nativeResourceState(
		ResourceStateFlags state)
	{
		D3D12_RESOURCE_STATES nativeState = D3D12_RESOURCE_STATE_COMMON;

		if ((state & ResourceStateFlags::VertexBuffer) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}

		if ((state & ResourceStateFlags::ConstantBuffer) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}

		if ((state & ResourceStateFlags::IndexBuffer) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}

		if ((state & ResourceStateFlags::ShaderResource) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}

		if ((state & ResourceStateFlags::UnorderedAccess) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		if ((state & ResourceStateFlags::RenderTarget) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		}

		if ((state & ResourceStateFlags::DepthWrite) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}

		if ((state & ResourceStateFlags::DepthRead) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_DEPTH_READ;
		}

		if ((state & ResourceStateFlags::CopySource) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		}

		if ((state & ResourceStateFlags::CopyDest) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_COPY_DEST;
		}

		if ((state & ResourceStateFlags::IndirectArgument) != ResourceStateFlags::Unknown)
		{
			nativeState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		}

		return nativeState;
	}
} // namespace spall::d3d12
