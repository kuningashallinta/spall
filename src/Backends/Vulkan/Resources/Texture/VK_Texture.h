// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Resources/Texture/ITexture.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <vk_mem_alloc.h>

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

	class Texture final : public SharedObject<ITexture>
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

		~Texture(void) override;

		RenderBackendType backendType(void) const override;
		TextureInfo info(void) const override;

	private:
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
	};
} // namespace spall::vk
