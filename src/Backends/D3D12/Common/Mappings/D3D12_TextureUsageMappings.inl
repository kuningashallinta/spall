// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::d3d12
{
	inline D3D12_RESOURCE_FLAGS textureUsageFlags(
		TextureUsageFlags usage)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		if ((usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if ((usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			if ((usage & TextureUsageFlags::Sampled) == TextureUsageFlags::None)
			{
				flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		if ((usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		return flags;
	}

	inline D3D12_FORMAT_SUPPORT1 requiredFormatSupport(
		TextureUsageFlags usage)
	{
		D3D12_FORMAT_SUPPORT1 support = D3D12_FORMAT_SUPPORT1_TEXTURE2D;

		if ((usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None)
		{
			support |= D3D12_FORMAT_SUPPORT1_RENDER_TARGET;
		}

		if ((usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)
		{
			support |= D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL;
		}

		if ((usage & TextureUsageFlags::Sampled) != TextureUsageFlags::None)
		{
			support |= D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE;
		}

		if ((usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
		{
			support |= D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW;
		}

		return support;
	}
} // namespace spall::d3d12
