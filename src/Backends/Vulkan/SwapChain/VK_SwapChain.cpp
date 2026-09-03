// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChain.h>

#include <src/Backends/Vulkan/Common/VK_EnumMappings.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>
#include <src/Backends/Vulkan/SwapChain/VK_SurfaceLifetime.h>
#include <src/Backends/Vulkan/SwapChain/VK_SwapChainGeneration.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace spall::vk
{
	std::optional<SwapChain::SurfaceFormatSelection> SwapChain::chooseSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats,
		Format requestedFormat)
	{
		const std::optional<VkFormat> requestedVkFormat = toVkFormat(requestedFormat);

		if (requestedVkFormat.has_value())
		{
			if ((availableFormats.size() == 1) and (availableFormats[0].format == VK_FORMAT_UNDEFINED))
			{
				return SurfaceFormatSelection {requestedVkFormat.value(), availableFormats[0].colorSpace, requestedFormat};
			}

			for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
			{
				if (availableFormat.format == requestedVkFormat.value())
				{
					return SurfaceFormatSelection {availableFormat.format, availableFormat.colorSpace, requestedFormat};
				}
			}
		}

		return std::nullopt;
	}

	VkExtent2D SwapChain::chooseSwapExtent(
		const VkSurfaceCapabilitiesKHR& capabilities,
		std::uint32_t width,
		std::uint32_t height)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}

		VkExtent2D extent = {};
		extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}

	VkCompositeAlphaFlagBitsKHR SwapChain::chooseCompositeAlpha(
		const VkSurfaceCapabilitiesKHR& capabilities)
	{
		const VkCompositeAlphaFlagBitsKHR candidates[] = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

		for (VkCompositeAlphaFlagBitsKHR candidate : candidates)
		{
			if ((capabilities.supportedCompositeAlpha & candidate) != 0)
			{
				return candidate;
			}
		}

		return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}

	SwapChain::SwapChain(
		Device& device,
		VkSurfaceKHR surface,
		std::uint32_t width,
		std::uint32_t height,
		Format format,
		PresentMode presentMode)
		: m_Device(&device), m_Surface(std::make_shared<SurfaceLifetime>(device, surface)), m_Width(width), m_Height(height), m_RequestedFormat(format), m_PresentMode(presentMode)
	{
	}

	SwapChain::~SwapChain()
	{
		SPALL_ASSERT(m_LiveFrameCount == 0);

		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE))
		{
			vkDeviceWaitIdle(m_Device->m_Device);
		}

		destroyFrameResources();
		destroyBackBuffers();
		m_Generation.reset();
		m_Surface.reset();
	}

	RenderBackendType SwapChain::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	Format SwapChain::format() const
	{
		return m_Format;
	}

	std::uint32_t SwapChain::frameCount() const
	{
		return static_cast<std::uint32_t>(m_FrameResources.size());
	}

	Status SwapChain::resize(
		std::uint32_t width,
		std::uint32_t height)
	{
		if (m_LiveFrameCount != 0)
		{
			return ERR_INVALID_STATE;
		}

		if ((width == 0) or (height == 0))
		{
			m_Width = width;
			m_Height = height;

			return {};
		}

		return recreate(width, height);
	}

	Status SwapChain::recreate(
		std::uint32_t width,
		std::uint32_t height)
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE) or (not m_Surface) or (m_Surface->m_Surface == VK_NULL_HANDLE))
		{
			return ERR_INVALID_STATE;
		}

		if (m_LiveFrameCount != 0)
		{
			return ERR_INVALID_STATE;
		}

		if ((width == 0) or (height == 0))
		{
			m_Width = width;
			m_Height = height;

			return {};
		}

		VkResult vkResult = vkDeviceWaitIdle(m_Device->m_Device);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		VkSurfaceCapabilitiesKHR capabilities = {};
		vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->m_PhysicalDevice, m_Surface->m_Surface, &capabilities);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		std::vector<VkSurfaceFormatKHR> surfaceFormats;

		do
		{
			std::uint32_t surfaceFormatCount = 0;
			vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
				m_Device->m_PhysicalDevice,
				m_Surface->m_Surface,
				&surfaceFormatCount,
				nullptr);

			if (vkResult != VK_SUCCESS)
			{
				return mapStatus(vkResult);
			}

			surfaceFormats.resize(surfaceFormatCount);

			if (surfaceFormatCount != 0)
			{
				vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
					m_Device->m_PhysicalDevice,
					m_Surface->m_Surface,
					&surfaceFormatCount,
					surfaceFormats.data());
				surfaceFormats.resize(surfaceFormatCount);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		if (surfaceFormats.empty())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		const std::optional<SurfaceFormatSelection> surfaceFormat = chooseSurfaceFormat(surfaceFormats, m_RequestedFormat);

		if (not surfaceFormat.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		constexpr VkImageUsageFlags requiredImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if ((capabilities.supportedUsageFlags & requiredImageUsage) != requiredImageUsage)
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		std::vector<VkPresentModeKHR> presentModes;

		do
		{
			std::uint32_t presentModeCount = 0;
			vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(
				m_Device->m_PhysicalDevice,
				m_Surface->m_Surface,
				&presentModeCount,
				nullptr);

			if (vkResult != VK_SUCCESS)
			{
				return mapStatus(vkResult);
			}

			presentModes.resize(presentModeCount);

			if (presentModeCount != 0)
			{
				vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(
					m_Device->m_PhysicalDevice,
					m_Surface->m_Surface,
					&presentModeCount,
					presentModes.data());
				presentModes.resize(presentModeCount);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		if (presentModes.empty())
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

		if (m_PresentMode == PresentMode::Immediate)
		{
			const auto immediateMode = std::find(
				presentModes.begin(),
				presentModes.end(),
				VK_PRESENT_MODE_IMMEDIATE_KHR);

			if (immediateMode == presentModes.end())
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		const VkExtent2D extent = chooseSwapExtent(capabilities, width, height);
		std::uint32_t imageCount = (std::max)(capabilities.minImageCount, 2u);

		if ((capabilities.maxImageCount != 0) and (imageCount > capabilities.maxImageCount))
		{
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface->m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat->VkSurfaceFormat;
		createInfo.imageColorSpace = surfaceFormat->ColorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = requiredImageUsage;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = chooseCompositeAlpha(capabilities);
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = m_Generation ? m_Generation->m_SwapChain : VK_NULL_HANDLE;

		VkSwapchainKHR newSwapChain = VK_NULL_HANDLE;
		vkResult = vkCreateSwapchainKHR(m_Device->m_Device, &createInfo, nullptr, &newSwapChain);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		std::vector<VkImage> images;

		do
		{
			std::uint32_t imageCountFromDriver = 0;
			vkResult = vkGetSwapchainImagesKHR(m_Device->m_Device, newSwapChain, &imageCountFromDriver, nullptr);

			if (vkResult != VK_SUCCESS)
			{
				vkDestroySwapchainKHR(m_Device->m_Device, newSwapChain, nullptr);
				return mapStatus(vkResult);
			}

			images.resize(imageCountFromDriver);

			if (imageCountFromDriver != 0)
			{
				vkResult = vkGetSwapchainImagesKHR(
					m_Device->m_Device,
					newSwapChain,
					&imageCountFromDriver,
					images.data());
				images.resize(imageCountFromDriver);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroySwapchainKHR(m_Device->m_Device, newSwapChain, nullptr);

			return mapStatus(vkResult);
		}

		if (images.empty())
		{
			vkDestroySwapchainKHR(m_Device->m_Device, newSwapChain, nullptr);

			return ERR_SWAP_CHAIN_CREATION_FAILED;
		}

		std::shared_ptr<SwapChainGeneration> newGeneration = std::make_shared<SwapChainGeneration>(m_Surface, newSwapChain);

		destroyFrameResources();
		destroyBackBuffers();
		m_Generation = std::move(newGeneration);
		m_Width = extent.width;
		m_Height = extent.height;
		m_Format = surfaceFormat->SelectedFormat;
		m_VkFormat = surfaceFormat->VkSurfaceFormat;
		m_ColorSpace = surfaceFormat->ColorSpace;
		m_CurrentFrameSlot = 0;

		Status error = createBackBuffers(images);

		if (error != SUCCESS)
		{
			destroyBackBuffers();
			destroyFrameResources();

			m_Generation.reset();

			m_Format = Format::Unknown;
			m_VkFormat = VK_FORMAT_UNDEFINED;
			m_ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			m_CurrentFrameSlot = 0;

			return error;
		}

		error = createFrameResources(static_cast<std::uint32_t>(images.size()));

		if (error != SUCCESS)
		{
			destroyBackBuffers();
			destroyFrameResources();

			m_Generation.reset();

			m_Format = Format::Unknown;
			m_VkFormat = VK_FORMAT_UNDEFINED;
			m_ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			m_CurrentFrameSlot = 0;

			return error;
		}

		m_RecreateBeforeAcquire = false;
		return {};
	}

	Status SwapChain::createFrameResources(
		std::uint32_t frameCount)
	{
		std::vector<FrameResources> frameResources(frameCount);

		for (FrameResources& frameResource : frameResources)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			const VkResult vkResult = vkCreateSemaphore(m_Device->m_Device, &semaphoreCreateInfo, nullptr, &frameResource.ImageAvailableSemaphore);

			if (vkResult != VK_SUCCESS)
			{
				for (FrameResources& createdResource : frameResources)
				{
					if (createdResource.ImageAvailableSemaphore != VK_NULL_HANDLE)
					{
						vkDestroySemaphore(m_Device->m_Device, createdResource.ImageAvailableSemaphore, nullptr);
					}
				}

				return mapStatus(vkResult);
			}
		}

		m_FrameResources = std::move(frameResources);

		return {};
	}

	void SwapChain::destroyFrameResources()
	{
		if ((not m_Device) or (m_Device->m_Device == VK_NULL_HANDLE))
		{
			m_FrameResources.clear();
			return;
		}

		for (FrameResources& frameResource : m_FrameResources)
		{
			frameResource.InFlightCommandList.reset();
			frameResource.InFlightSubmissionSerial = 0;

			if (frameResource.ImageAvailableSemaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device->m_Device, frameResource.ImageAvailableSemaphore, nullptr);
			}
		}

		m_FrameResources.clear();
	}

	Status SwapChain::createBackBuffers(
		const std::vector<VkImage>& images)
	{
		SPALL_ASSERT(m_Generation);

		std::vector<BackBuffer> backBuffers;
		backBuffers.reserve(images.size());

		for (VkImage image : images)
		{
			TextureInfo textureInfo = {};
			textureInfo.Width = m_Width;
			textureInfo.Height = m_Height;
			textureInfo.Format = m_Format;
			textureInfo.Usage = TextureUsageFlags::ColorAttachment | TextureUsageFlags::TransferDestination;
			textureInfo.InitialState = ResourceStateFlags::Present;
			textureInfo.KeepInitialState = true;

			BackBuffer backBuffer = {};
			Texture::SwapChainBinding swapChainBinding = {};
			swapChainBinding.IsSwapChainTexture = true;
			swapChainBinding.Owner = this;
			swapChainBinding.Generation = m_Generation;

			Texture2D* texture = new Texture2D(
				*m_Device,
				textureInfo,
				image,
				VK_NULL_HANDLE,
				VK_IMAGE_ASPECT_COLOR_BIT,
				false,
				std::move(swapChainBinding));

			backBuffer.Texture = Resource<Texture2D>(texture);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;

			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.image = image;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = m_VkFormat;
			viewCreateInfo.subresourceRange = subresourceRange;

			VkImageView imageView = VK_NULL_HANDLE;
			const VkResult vkResult = vkCreateImageView(m_Device->m_Device, &viewCreateInfo, nullptr, &imageView);

			if (vkResult != VK_SUCCESS)
			{
				for (BackBuffer& createdBackBuffer : backBuffers)
				{
					if (createdBackBuffer.RenderFinishedSemaphore != VK_NULL_HANDLE)
					{
						vkDestroySemaphore(m_Device->m_Device, createdBackBuffer.RenderFinishedSemaphore, nullptr);
					}
				}

				return mapStatus(vkResult);
			}

			TextureView::Subresources subresources = {};
			subresources.Aspects = TextureAspectFlags::Color;

			TextureView* view = new TextureView(*backBuffer.Texture, subresources, imageView, true);

			backBuffer.View = Resource<TextureView>(view);

			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			const VkResult semaphoreResult = vkCreateSemaphore(
				m_Device->m_Device,
				&semaphoreCreateInfo,
				nullptr,
				&backBuffer.RenderFinishedSemaphore);

			if (semaphoreResult != VK_SUCCESS)
			{
				for (BackBuffer& createdBackBuffer : backBuffers)
				{
					if (createdBackBuffer.RenderFinishedSemaphore != VK_NULL_HANDLE)
					{
						vkDestroySemaphore(m_Device->m_Device, createdBackBuffer.RenderFinishedSemaphore, nullptr);
					}
				}

				return mapStatus(semaphoreResult);
			}

			backBuffers.push_back(std::move(backBuffer));
		}

		m_BackBuffers = std::move(backBuffers);

		return {};
	}

	void SwapChain::destroyBackBuffers()
	{
		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE))
		{
			for (BackBuffer& backBuffer : m_BackBuffers)
			{
				backBuffer.InFlightCommandList.reset();
				backBuffer.InFlightSubmissionSerial = 0;

				if (backBuffer.RenderFinishedSemaphore != VK_NULL_HANDLE)
				{
					vkDestroySemaphore(m_Device->m_Device, backBuffer.RenderFinishedSemaphore, nullptr);
				}
			}
		}

		m_BackBuffers.clear();
	}
} // namespace spall::vk
