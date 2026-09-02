// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Validation/Common/TextureValidation.h>

#include <utility>

namespace spall::d3d12
{
	Texture* textureStorage(
		ITexture& texture)
	{
		if (texture.backendType() != RenderBackendType::D3D12)
		{
			return nullptr;
		}

		switch (texture.type())
		{
			case TextureType::Texture1D:
			{
				return static_cast<Texture1D*>(&texture);
			}

			case TextureType::Texture2D:
			{
				return static_cast<Texture2D*>(&texture);
			}

			case TextureType::Texture3D:
			{
				return static_cast<Texture3D*>(&texture);
			}
		}

		return nullptr;
	}

	Texture* textureStorage(
		ITexture* texture)
	{
		return (texture != nullptr) ? textureStorage(*texture) : nullptr;
	}

	ITexture* textureInterface(
		Texture& storage)
	{
		switch (storage.m_Info.Type)
		{
			case TextureType::Texture1D:
			{
				return static_cast<Texture1D*>(&storage);
			}

			case TextureType::Texture2D:
			{
				return static_cast<Texture2D*>(&storage);
			}

			case TextureType::Texture3D:
			{
				return static_cast<Texture3D*>(&storage);
			}
		}

		return nullptr;
	}

	Texture::Texture(
		Device& device,
		const TextureInfo& info,
		ComPtr<ID3D12Resource> resource,
		SwapChain* swapChain)
		: m_Device(&device), m_SwapChain(swapChain), m_Info(info), m_Resource(std::move(resource)),
		m_IsSwapChainTexture(swapChain != nullptr)
	{
		m_SubresourceStates.assign(textureSubresourceCount(info), info.InitialState);

		if (info.DebugName != nullptr)
		{
			m_DebugName = info.DebugName;
			m_Info.DebugName = m_DebugName.c_str();
		}
		else
		{
			m_Info.DebugName = nullptr;
		}
	}

	Texture::~Texture() = default;

	Texture1D::Texture1D(
		Device& device,
		const TextureInfo& info,
		ComPtr<ID3D12Resource> resource)
		: Texture(device, info, std::move(resource))
	{
	}

	RenderBackendType Texture1D::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	TextureType Texture1D::type() const
	{
		return TextureType::Texture1D;
	}

	TextureInfo Texture1D::info() const
	{
		return m_Info;
	}

	std::uint32_t Texture1D::width() const
	{
		return m_Info.Width;
	}

	std::uint32_t Texture1D::arrayLayers() const
	{
		return m_Info.ArrayLayers;
	}

	Texture2D::Texture2D(
		Device& device,
		const TextureInfo& info,
		ComPtr<ID3D12Resource> resource,
		SwapChain* swapChain)
		: Texture(device, info, std::move(resource), swapChain)
	{
	}

	RenderBackendType Texture2D::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	TextureType Texture2D::type() const
	{
		return TextureType::Texture2D;
	}

	TextureInfo Texture2D::info() const
	{
		return m_Info;
	}

	std::uint32_t Texture2D::width() const
	{
		return m_Info.Width;
	}

	std::uint32_t Texture2D::height() const
	{
		return m_Info.Height;
	}

	std::uint32_t Texture2D::arrayLayers() const
	{
		return m_Info.ArrayLayers;
	}

	std::uint32_t Texture2D::sampleCount() const
	{
		return m_Info.SampleCount;
	}

	bool Texture2D::isCubemap() const
	{
		return m_Info.Cubemap;
	}

	Texture3D::Texture3D(
		Device& device,
		const TextureInfo& info,
		ComPtr<ID3D12Resource> resource)
		: Texture(device, info, std::move(resource))
	{
	}

	RenderBackendType Texture3D::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	TextureType Texture3D::type() const
	{
		return TextureType::Texture3D;
	}

	TextureInfo Texture3D::info() const
	{
		return m_Info;
	}

	std::uint32_t Texture3D::width() const
	{
		return m_Info.Width;
	}

	std::uint32_t Texture3D::height() const
	{
		return m_Info.Height;
	}

	std::uint32_t Texture3D::depth() const
	{
		return m_Info.Depth;
	}
} // namespace spall::d3d12
