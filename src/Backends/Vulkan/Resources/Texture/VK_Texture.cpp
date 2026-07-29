#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Common/VK_DebugName.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChainGeneration.h>
#include <src/Validation/Common/TextureValidation.h>

#include <utility>

namespace spall::vk
{
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

	RenderBackendType Texture::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	TextureInfo Texture::info() const
	{
		return m_Info;
	}
} // namespace spall::vk
