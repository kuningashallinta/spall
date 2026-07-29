namespace spall::d3d12
{
	inline FormatCapabilities formatCapabilities(
		Format format,
		D3D12_FORMAT_SUPPORT1 support)
	{
		FormatCapabilities capabilities = {};

		if (isVertexFormat(format) and ((support & D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER) != 0))
		{
			capabilities.SupportsVertexInput = true;
		}

		if (not isTextureFormat(format) or ((support & D3D12_FORMAT_SUPPORT1_TEXTURE2D) == 0))
		{
			return capabilities;
		}

		capabilities.SupportedTextureUsages |= TextureUsageFlags::TransferSource;
		capabilities.SupportedTextureUsages |= TextureUsageFlags::TransferDestination;

		if (isColorFormat(format) and ((support & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::ColorAttachment;
			capabilities.SupportsBlending = ((support & D3D12_FORMAT_SUPPORT1_BLENDABLE) != 0);
		}

		if (isDepthFormat(format) and ((support & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::DepthStencilAttachment;
		}

		if (not isDepthFormat(format) and ((support & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::Sampled;
			capabilities.SupportsLinearFiltering = true;
		}

		if (not isDepthFormat(format) and ((support & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0))
		{
			capabilities.SupportedTextureUsages |= TextureUsageFlags::Storage;
		}

		return capabilities;
	}
} // namespace spall::d3d12
