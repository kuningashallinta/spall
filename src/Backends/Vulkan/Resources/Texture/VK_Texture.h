// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Resources/Texture/ITexture1D.h>
#include <spall/Resources/Texture/ITexture2D.h>
#include <spall/Resources/Texture/ITexture3D.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <vk_mem_alloc.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace spall::vk
{
	class Device;
	class SwapChain;
	class GraphicsQueue;
	class CommandList;
	class TextureView;
	class ResourceSet;
	class SwapChainGeneration;

	/// Image storage shared by every texture dimension.
	///
	/// This is not an ITexture. Texture1D, Texture2D and Texture3D each implement
	/// one public interface and inherit this storage, so backend code holding an
	/// ITexture reaches the image through textureStorage().
	class Texture
	{
	public:
		struct SwapChainBinding
		{
			bool IsSwapChainTexture = false;
			SwapChain* Owner = nullptr;
			std::shared_ptr<SwapChainGeneration> Generation;
		};

		Texture(
			Device& device,
			const TextureInfo& info,
			VkImage image,
			VmaAllocation allocation,
			VkImageAspectFlags aspectMask,
			bool ownsImage,
			SwapChainBinding swapChainBinding = {});

		~Texture(void);

	protected:
		struct SubresourceState
		{
			ResourceStateFlags State = ResourceStateFlags::Unknown;
			bool Initialized = false;
		};

		Resource<Device> m_Device;
		SwapChain* m_SwapChain = nullptr;
		std::shared_ptr<SwapChainGeneration> m_SwapChainGeneration;

		std::string m_DebugName;
		TextureInfo m_Info = {};

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;

		VkImageAspectFlags m_AspectMask = 0;
		std::vector<SubresourceState> m_SubresourceStates;
		ResourceStateFlags m_PermanentState = ResourceStateFlags::Unknown;

		bool m_OwnsImage = true;
		bool m_IsSwapChainTexture = false;

	private:
		friend class Device;
		friend class SwapChain;
		friend class GraphicsQueue;
		friend class CommandList;
		friend class ResourceStateTracker;
		friend class TextureView;
		friend class ResourceSet;

		friend ITexture* textureInterface(Texture& storage);
	};

	class Texture1D final : public SharedObject<ITexture1D>, public Texture
	{
	public:
		Texture1D(
			Device& device,
			const TextureInfo& info,
			VkImage image,
			VmaAllocation allocation,
			VkImageAspectFlags aspectMask,
			bool ownsImage);

		RenderBackendType backendType(void) const override;
		TextureType type(void) const override;
		TextureInfo info(void) const override;

		std::uint32_t width(void) const override;
		std::uint32_t arrayLayers(void) const override;
	};

	class Texture2D final : public SharedObject<ITexture2D>, public Texture
	{
	public:
		Texture2D(
			Device& device,
			const TextureInfo& info,
			VkImage image,
			VmaAllocation allocation,
			VkImageAspectFlags aspectMask,
			bool ownsImage,
			SwapChainBinding swapChainBinding = {});

		RenderBackendType backendType(void) const override;
		TextureType type(void) const override;
		TextureInfo info(void) const override;

		std::uint32_t width(void) const override;
		std::uint32_t height(void) const override;
		std::uint32_t arrayLayers(void) const override;
		std::uint32_t sampleCount(void) const override;
		bool isCubemap(void) const override;
	};

	class Texture3D final : public SharedObject<ITexture3D>, public Texture
	{
	public:
		Texture3D(
			Device& device,
			const TextureInfo& info,
			VkImage image,
			VmaAllocation allocation,
			VkImageAspectFlags aspectMask,
			bool ownsImage);

		RenderBackendType backendType(void) const override;
		TextureType type(void) const override;
		TextureInfo info(void) const override;

		std::uint32_t width(void) const override;
		std::uint32_t height(void) const override;
		std::uint32_t depth(void) const override;
	};

	/// Gets the image storage behind a texture of any dimension.
	/// Returns null when the texture belongs to a different backend.
	Texture* textureStorage(ITexture& texture);

	Texture* textureStorage(ITexture* texture);

	/// Gets the interface object that owns a texture's image storage.
	ITexture* textureInterface(Texture& storage);
} // namespace spall::vk
