// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Common/VK_DebugName.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChainGeneration.h>
#include <src/Validation/Common/TextureValidation.h>

#include <utility>

namespace spall::vk
{
	Texture* textureStorage(
		ITexture& texture)
	{
		if (texture.backendType() != RenderBackendType::Vulkan)
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
		VkImage image,
		VmaAllocation allocation,
		VkImageAspectFlags aspectMask,
		bool ownsImage,
		SwapChainBinding swapChainBinding)
		: m_Device(&device),
		  m_SwapChain(swapChainBinding.Owner),
		  m_SwapChainGeneration(std::move(swapChainBinding.Generation)),
		  m_DebugName(info.DebugName != nullptr ? info.DebugName : ""),
		  m_Info(info),
		  m_Image(image),
		  m_Allocation(allocation),
		  m_AspectMask(aspectMask),
		  m_SubresourceStates(textureSubresourceCount(info), SubresourceState {info.InitialState, false}),
		  m_OwnsImage(ownsImage),
		  m_IsSwapChainTexture(swapChainBinding.IsSwapChainTexture)
	{
		m_Info.DebugName = m_DebugName.empty() ? nullptr : m_DebugName.c_str();
		setDebugName(
			m_Device->m_Device,
			VK_OBJECT_TYPE_IMAGE,
			reinterpret_cast<std::uint64_t>(m_Image),
			m_Info.DebugName);
	}

	Texture::~Texture()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE) or not m_OwnsImage)
		{
			return;
		}

		if (m_Image != VK_NULL_HANDLE)
		{
			vmaDestroyImage(m_Device->m_Allocator, m_Image, m_Allocation);
		}
	}

	Texture1D::Texture1D(
		Device& device,
		const TextureInfo& info,
		VkImage image,
		VmaAllocation allocation,
		VkImageAspectFlags aspectMask,
		bool ownsImage)
		: Texture(device, info, image, allocation, aspectMask, ownsImage)
	{
	}

	RenderBackendType Texture1D::backendType() const
	{
		return RenderBackendType::Vulkan;
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
		VkImage image,
		VmaAllocation allocation,
		VkImageAspectFlags aspectMask,
		bool ownsImage,
		SwapChainBinding swapChainBinding)
		: Texture(device, info, image, allocation, aspectMask, ownsImage, std::move(swapChainBinding))
	{
	}

	RenderBackendType Texture2D::backendType() const
	{
		return RenderBackendType::Vulkan;
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
		VkImage image,
		VmaAllocation allocation,
		VkImageAspectFlags aspectMask,
		bool ownsImage)
		: Texture(device, info, image, allocation, aspectMask, ownsImage)
	{
	}

	RenderBackendType Texture3D::backendType() const
	{
		return RenderBackendType::Vulkan;
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
} // namespace spall::vk
