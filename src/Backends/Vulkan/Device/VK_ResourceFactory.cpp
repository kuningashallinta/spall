// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/Device/VK_Device.h>

#include <spall/Common/Assert.h>
#include <src/Backends/Vulkan/Common/VK_BackendCast.h>
#include <src/Backends/Vulkan/Common/VK_EnumMappings.h>
#include <src/Backends/Vulkan/Framebuffer/VK_Framebuffer.h>
#include <src/Backends/Vulkan/Queue/VK_GraphicsQueue.h>
#include <src/Backends/Vulkan/Resources/AccelerationStructure/VK_AccelerationStructure.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_Buffer.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_BufferState.h>
#include <src/Backends/Vulkan/Resources/Query/VK_QueryPool.h>
#include <src/Backends/Vulkan/Resources/Sampler/VK_Sampler.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>
#include <src/Validation/Backends/Vulkan/BufferValidation.h>
#include <src/Validation/Common.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace spall::vk
{
	VkImageAspectFlags Device::imageAspectMask(
		Format format)
	{
		const TextureAspectFlags defaultAspects = vk::defaultAspects(format);
		const VkImageAspectFlags aspectMask = vk::aspectMask(defaultAspects);

		return (aspectMask != 0) ? aspectMask : VK_IMAGE_ASPECT_COLOR_BIT;
	}

	Status Device::createTexture(
		const TextureCreateInfo& info,
		Resource<ITexture>* texture)
	{
		if (texture == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateTextureCreateInfo(info));

		const std::optional<VkFormat> format = toVkFormat(info.Format);

		if (not format.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		const VkPhysicalDeviceLimits& limits = m_Properties.limits;

		if ((info.Width > limits.maxImageDimension2D) or (info.Height > limits.maxImageDimension2D) or
			(info.ArrayLayers > limits.maxImageArrayLayers) or
			((((info.Usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None) or
				 ((info.Usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)) and
				((info.Width > limits.maxFramebufferWidth) or (info.Height > limits.maxFramebufferHeight))))
		{
			return ERR_INVALID_SIZE;
		}

		if (info.Cubemap and (info.Width > limits.maxImageDimensionCube))
		{
			return ERR_INVALID_SIZE;
		}

		if ((m_Limits.SupportedSampleCounts & info.SampleCount) == 0)
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		VkFormatFeatureFlags requiredFormatFeatures = 0;

		if ((info.Usage & TextureUsageFlags::ColorAttachment) != TextureUsageFlags::None)
		{
			requiredFormatFeatures |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		}

		if ((info.Usage & TextureUsageFlags::DepthStencilAttachment) != TextureUsageFlags::None)
		{
			requiredFormatFeatures |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		if ((info.Usage & TextureUsageFlags::Sampled) != TextureUsageFlags::None)
		{
			requiredFormatFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		}

		if ((info.Usage & TextureUsageFlags::Storage) != TextureUsageFlags::None)
		{
			requiredFormatFeatures |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
		}

		if ((info.Usage & TextureUsageFlags::TransferSource) != TextureUsageFlags::None)
		{
			requiredFormatFeatures |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
		}

		if ((info.Usage & TextureUsageFlags::TransferDestination) != TextureUsageFlags::None)
		{
			requiredFormatFeatures |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
		}

		VkFormatProperties formatProperties = {};
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format.value(), &formatProperties);

		if ((formatProperties.optimalTilingFeatures & requiredFormatFeatures) != requiredFormatFeatures)
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = (info.Depth > 1) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = info.Width;
		imageCreateInfo.extent.height = info.Height;
		imageCreateInfo.extent.depth = info.Depth;
		imageCreateInfo.mipLevels = info.MipLevels;
		imageCreateInfo.arrayLayers = info.ArrayLayers;
		imageCreateInfo.flags = info.Cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		imageCreateInfo.format = format.value();
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = vulkanImageUsageFlags(info.Usage);
		imageCreateInfo.samples = static_cast<VkSampleCountFlagBits>(info.SampleCount);
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		const VkResult vkResult = vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, nullptr);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		const VkImageAspectFlags aspectMask = imageAspectMask(info.Format);

		Texture* vkTexture = new Texture(
			*this,
			TextureInfo {info.Width, info.Height, info.Depth, info.MipLevels, info.ArrayLayers, info.SampleCount, info.Cubemap, info.Format, info.Usage, info.InitialState, info.KeepInitialState, info.DebugName},
			image,
			allocation,
			aspectMask,
			true);

		*texture = Resource<ITexture>(vkTexture);

		return {};
	}

	Status Device::createTextureView(
		const TextureViewCreateInfo& info,
		Resource<ITextureView>* textureView)
	{
		if (textureView == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateTextureViewCreateInfo(info));

		Texture* texture = dynamic_cast<Texture*>(info.Texture);

		if (texture == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (texture->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		const Format format = (info.Format == Format::Unknown) ? texture->m_Info.Format : info.Format;
		const std::optional<VkFormat> vkFormat = toVkFormat(format);
		const std::uint32_t mipLevels = (info.MipLevels != 0)
			? info.MipLevels
			: texture->m_Info.MipLevels - info.BaseMipLevel;
		const std::uint32_t arrayLayers = (info.ArrayLayers != 0)
			? info.ArrayLayers
			: texture->m_Info.ArrayLayers - info.BaseArrayLayer;

		if (not vkFormat.has_value())
		{
			return ERR_UNSUPPORTED_FORMAT;
		}

		TextureAspectFlags aspects = info.Aspects;

		if (aspects == TextureAspectFlags::None)
		{
			aspects = defaultAspects(format);
		}

		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = texture->m_Image;
		viewCreateInfo.viewType = (texture->m_Info.Depth > 1)
			? VK_IMAGE_VIEW_TYPE_3D
			: (info.Cubemap
					  ? ((arrayLayers > 6) ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE)
					  : ((arrayLayers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D));
		viewCreateInfo.format = vkFormat.value();
		viewCreateInfo.subresourceRange.aspectMask = aspectMask(aspects);
		viewCreateInfo.subresourceRange.baseMipLevel = info.BaseMipLevel;
		viewCreateInfo.subresourceRange.levelCount = mipLevels;
		viewCreateInfo.subresourceRange.baseArrayLayer = info.BaseArrayLayer;
		viewCreateInfo.subresourceRange.layerCount = arrayLayers;

		VkImageView view = VK_NULL_HANDLE;
		const VkResult vkResult = vkCreateImageView(m_Device, &viewCreateInfo, nullptr, &view);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		TextureView::Subresources subresources = {};
		subresources.Aspects = aspects;
		subresources.BaseMipLevel = info.BaseMipLevel;
		subresources.MipLevels = mipLevels;
		subresources.BaseArrayLayer = info.BaseArrayLayer;
		subresources.ArrayLayers = arrayLayers;
		subresources.Cubemap = info.Cubemap;

		TextureView* vkTextureView = new TextureView(*texture, subresources, view, true);

		*textureView = Resource<ITextureView>(vkTextureView);

		return {};
	}

	Status Device::createFramebuffer(
		const FramebufferCreateInfo& createInfo,
		Resource<IFramebuffer>* framebuffer)
	{
		if (framebuffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateFramebufferCreateInfo(createInfo));

		TextureView* colorViews[MaxColorAttachments] = {};
		FramebufferInfo info = {};
		info.ColorFormatCount = createInfo.ColorAttachmentCount;

		bool haveDimensions = false;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < createInfo.ColorAttachmentCount; ++attachmentIndex)
		{
			TextureView* colorView = dynamic_cast<TextureView*>(createInfo.ColorAttachments[attachmentIndex]);

			if ((colorView == nullptr) or (not colorView->m_Texture) or (colorView->m_Texture->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			colorViews[attachmentIndex] = colorView;
			info.ColorFormats[attachmentIndex] = colorView->m_Texture->m_Info.Format;

			if (not haveDimensions)
			{
				info.Width = mipLevelExtent(colorView->m_Texture->m_Info.Width, colorView->m_BaseMipLevel);
				info.Height = mipLevelExtent(colorView->m_Texture->m_Info.Height, colorView->m_BaseMipLevel);
				haveDimensions = true;
			}
			else if ((info.Width != mipLevelExtent(colorView->m_Texture->m_Info.Width, colorView->m_BaseMipLevel)) or
				(info.Height != mipLevelExtent(colorView->m_Texture->m_Info.Height, colorView->m_BaseMipLevel)))
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		TextureView* depthView = nullptr;

		if (createInfo.DepthAttachment != nullptr)
		{
			depthView = dynamic_cast<TextureView*>(createInfo.DepthAttachment);

			if ((depthView == nullptr) or (not depthView->m_Texture) or (depthView->m_Texture->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			info.DepthFormat = depthView->m_Texture->m_Info.Format;

			if (not haveDimensions)
			{
				info.Width = mipLevelExtent(depthView->m_Texture->m_Info.Width, depthView->m_BaseMipLevel);
				info.Height = mipLevelExtent(depthView->m_Texture->m_Info.Height, depthView->m_BaseMipLevel);
				haveDimensions = true;
			}
			else if ((info.Width != mipLevelExtent(depthView->m_Texture->m_Info.Width, depthView->m_BaseMipLevel)) or
				(info.Height != mipLevelExtent(depthView->m_Texture->m_Info.Height, depthView->m_BaseMipLevel)))
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		TextureView* resolveViews[MaxColorAttachments] = {};
		info.SampleCount = framebufferSampleCount(createInfo);

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < createInfo.ColorAttachmentCount; ++attachmentIndex)
		{
			if (createInfo.ResolveAttachments[attachmentIndex] == nullptr)
			{
				continue;
			}

			TextureView* resolveView = dynamic_cast<TextureView*>(createInfo.ResolveAttachments[attachmentIndex]);

			if ((resolveView == nullptr) or (not resolveView->m_Texture) or (resolveView->m_Texture->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			resolveViews[attachmentIndex] = resolveView;
		}

		VkImageView attachments[(MaxColorAttachments * 2) + 1] = {};
		std::uint32_t attachmentCount = 0;

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < createInfo.ColorAttachmentCount; ++attachmentIndex)
		{
			attachments[attachmentCount++] = colorViews[attachmentIndex]->m_View;
		}

		if (depthView != nullptr)
		{
			attachments[attachmentCount++] = depthView->m_View;
		}

		for (std::uint32_t attachmentIndex = 0; attachmentIndex < createInfo.ColorAttachmentCount; ++attachmentIndex)
		{
			if (resolveViews[attachmentIndex] != nullptr)
			{
				attachments[attachmentCount++] = resolveViews[attachmentIndex]->m_View;
			}
		}

		const VkRenderPass compatibleRenderPass = renderPass(info.ColorFormats, info.ColorFormatCount, info.DepthFormat, info.SampleCount);

		if (compatibleRenderPass == VK_NULL_HANDLE)
		{
			return ERR_BACKEND_FAILURE;
		}

		VkFramebuffer framebufferHandle = VK_NULL_HANDLE;
		SPALL_TRY(createTransientFramebuffer(compatibleRenderPass, attachments, attachmentCount, info.Width, info.Height, framebufferHandle));

		Framebuffer* vkFramebuffer = new Framebuffer(*this, info, colorViews, createInfo.ColorAttachmentCount, depthView, resolveViews);
		vkFramebuffer->m_Framebuffer = framebufferHandle;

		*framebuffer = Resource<IFramebuffer>(vkFramebuffer);

		return {};
	}

	Status Device::createBuffer(
		const BufferCreateInfo& info,
		Resource<IBuffer>* buffer)
	{
		if (buffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateBufferCreateInfo(info));
		SPALL_TRY(BufferValidation::validateCreateInfo(info, m_Properties.limits));

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = info.Size;
		bufferCreateInfo.usage = vulkanBufferUsageFlags(info.Usage, m_RayTracingEnabled);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (info.CpuAccess == MemoryAccess::Write)
		{
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if (info.CpuAccess == MemoryAccess::Read)
		{
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		VkBuffer vkBuffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		const VkResult vkResult = vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &allocationCreateInfo, &vkBuffer, &allocation, nullptr);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		const BufferInfo bufferInfo {info.Size, info.Usage, info.CpuAccess, info.InitialState, info.KeepInitialState, info.DebugName};

		Buffer* createdBuffer = new Buffer(*this, bufferInfo, vkBuffer, allocation, info.InitialState);

		*buffer = Resource<IBuffer>(createdBuffer);

		return {};
	}

	Status Device::createBufferWithData(
		const BufferCreateInfo& info,
		std::span<const std::byte> data,
		Resource<IBuffer>* buffer)
	{
		if (buffer == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateBufferCreateInfo(info));
		SPALL_TRY(BufferValidation::validateCreateInfo(info, m_Properties.limits));

		if (data.empty())
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (data.size() != info.Size)
		{
			return ERR_INVALID_SIZE;
		}

		if (info.CpuAccess == MemoryAccess::Write)
		{
			Resource<IBuffer> writableBuffer;
			SPALL_TRY(createBuffer(info, &writableBuffer));
			SPALL_TRY(writeBuffer(*writableBuffer, data, 0));

			*buffer = std::move(writableBuffer);

			return {};
		}

		BufferCreateInfo stagingInfo = {};
		stagingInfo.Size = info.Size;
		stagingInfo.Usage = BufferUsageFlags::TransferSource;
		stagingInfo.CpuAccess = MemoryAccess::Write;

		Resource<IBuffer> staging;
		SPALL_TRY(createBuffer(stagingInfo, &staging));

		SPALL_TRY(writeBuffer(*staging, data, 0));

		Buffer* stagingBuffer = dynamic_cast<Buffer*>(staging.get());

		if (stagingBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if ((m_GraphicsQueue == nullptr) or (m_GraphicsQueue->m_NativeQueue == VK_NULL_HANDLE))
		{
			return ERR_INVALID_STATE;
		}

		const BufferInfo bufferInfo {info.Size, info.Usage, info.CpuAccess, info.InitialState, info.KeepInitialState, info.DebugName};
		const ResourceStateFlags uploadedState = info.InitialState;
		const std::optional<BufferStateInfo> uploadedStateInfo = vulkanBufferState(uploadedState);

		if (not uploadedStateInfo.has_value())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		const BufferUsageFlags nativeUsage = info.Usage | BufferUsageFlags::TransferDestination;

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = info.Size;
		bufferCreateInfo.usage = vulkanBufferUsageFlags(nativeUsage, m_RayTracingEnabled);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkBuffer vkBuffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VkResult vkResult = vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &allocationCreateInfo, &vkBuffer, &allocation, nullptr);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		VkCommandBuffer commandBuffer = allocateCommandBuffer();

		if (commandBuffer == VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Allocator, vkBuffer, allocation);

			return ERR_OUT_OF_MEMORY;
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);

		if (vkResult != VK_SUCCESS)
		{
			freeCommandBuffer(commandBuffer);
			vmaDestroyBuffer(m_Allocator, vkBuffer, allocation);

			return mapVulkanStatus(vkResult);
		}

		VkBufferMemoryBarrier transferBarriers[2] = {};
		transferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		transferBarriers[0].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		transferBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		transferBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transferBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transferBarriers[0].buffer = stagingBuffer->m_Buffer;
		transferBarriers[0].offset = 0;
		transferBarriers[0].size = VK_WHOLE_SIZE;

		transferBarriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		transferBarriers[1].srcAccessMask = 0;
		transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		transferBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transferBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transferBarriers[1].buffer = vkBuffer;
		transferBarriers[1].offset = 0;
		transferBarriers[1].size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			2,
			transferBarriers,
			0,
			nullptr);

		VkBufferCopy copyRegion = {};
		copyRegion.size = info.Size;

		vkCmdCopyBuffer(commandBuffer, stagingBuffer->m_Buffer, vkBuffer, 1, &copyRegion);

		if (uploadedState != ResourceStateFlags::CopyDest)
		{
			VkBufferMemoryBarrier uploadBarrier = {};
			uploadBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			uploadBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			uploadBarrier.dstAccessMask = uploadedStateInfo->access;
			uploadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			uploadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			uploadBarrier.buffer = vkBuffer;
			uploadBarrier.offset = 0;
			uploadBarrier.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				uploadedStateInfo->stage,
				0,
				0,
				nullptr,
				1,
				&uploadBarrier,
				0,
				nullptr);
		}

		vkResult = vkEndCommandBuffer(commandBuffer);

		if (vkResult != VK_SUCCESS)
		{
			freeCommandBuffer(commandBuffer);
			vmaDestroyBuffer(m_Allocator, vkBuffer, allocation);

			return mapVulkanStatus(vkResult);
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence uploadFence = VK_NULL_HANDLE;
		vkResult = vkCreateFence(m_Device, &fenceInfo, nullptr, &uploadFence);

		if (vkResult != VK_SUCCESS)
		{
			freeCommandBuffer(commandBuffer);
			vmaDestroyBuffer(m_Allocator, vkBuffer, allocation);

			return mapVulkanStatus(vkResult);
		}

		vkResult = vkQueueSubmit(m_GraphicsQueue->m_NativeQueue, 1, &submitInfo, uploadFence);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroyFence(m_Device, uploadFence, nullptr);

			freeCommandBuffer(commandBuffer);

			vmaDestroyBuffer(m_Allocator, vkBuffer, allocation);

			return mapVulkanStatus(vkResult);
		}

		vkResult = vkWaitForFences(m_Device, 1, &uploadFence, VK_TRUE, UINT64_MAX);

		vkDestroyFence(m_Device, uploadFence, nullptr);

		freeCommandBuffer(commandBuffer);

		if (vkResult != VK_SUCCESS)
		{
			vmaDestroyBuffer(m_Allocator, vkBuffer, allocation);

			return mapVulkanStatus(vkResult);
		}

		Buffer* createdBuffer = new Buffer(*this, bufferInfo, vkBuffer, allocation, uploadedState);

		*buffer = Resource<IBuffer>(createdBuffer);

		return {};
	}

	Status Device::writeBuffer(
		IBuffer& buffer,
		std::span<const std::byte> data,
		std::uint32_t offset)
	{
		if (data.empty())
		{
			return ERR_INVALID_SIZE;
		}

		Buffer* backendBuffer = dynamic_cast<Buffer*>(&buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendBuffer->m_Info.CpuAccess != MemoryAccess::Write)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((offset > backendBuffer->m_Info.Size) or (data.size() > backendBuffer->m_Info.Size - offset))
		{
			return ERR_INVALID_RANGE;
		}

		void* mappedData = nullptr;
		const VkResult vkResult = vmaMapMemory(m_Allocator, backendBuffer->m_Allocation, &mappedData);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		std::memcpy(static_cast<std::uint8_t*>(mappedData) + offset, data.data(), data.size());
		vmaUnmapMemory(m_Allocator, backendBuffer->m_Allocation);

		return {};
	}

	Status Device::readBuffer(
		IBuffer& buffer,
		std::span<std::byte> data,
		std::uint32_t offset)
	{
		if (data.empty())
		{
			return ERR_INVALID_SIZE;
		}

		Buffer* backendBuffer = dynamic_cast<Buffer*>(&buffer);

		if (backendBuffer == nullptr)
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		if (backendBuffer->m_Device.get() != this)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (backendBuffer->m_Info.CpuAccess != MemoryAccess::Read)
		{
			return ERR_INVALID_USAGE_FLAGS;
		}

		if ((offset > backendBuffer->m_Info.Size) or (data.size() > backendBuffer->m_Info.Size - offset))
		{
			return ERR_INVALID_RANGE;
		}

		void* mappedData = nullptr;
		const VkResult vkResult = vmaMapMemory(m_Allocator, backendBuffer->m_Allocation, &mappedData);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		std::memcpy(data.data(), static_cast<const std::uint8_t*>(mappedData) + offset, data.size());
		vmaUnmapMemory(m_Allocator, backendBuffer->m_Allocation);

		return {};
	}

	Status Device::createSampler(
		const SamplerCreateInfo& info,
		Resource<ISampler>* sampler)
	{
		if (sampler == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateSamplerCreateInfo(info));

		const bool anisotropic = info.MaxAnisotropy > 1.0f;

		if (anisotropic and (m_SupportedFeatures.samplerAnisotropy == VK_FALSE))
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		const float anisotropyLimit = m_Properties.limits.maxSamplerAnisotropy;

		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = vulkanSamplerFilter(info.MagFilter);
		createInfo.minFilter = vulkanSamplerFilter(info.MinFilter);
		createInfo.mipmapMode = (info.MipFilter == Filter::Nearest) ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.addressModeU = vulkanSamplerAddressMode(info.AddressModeU);
		createInfo.addressModeV = vulkanSamplerAddressMode(info.AddressModeV);
		createInfo.addressModeW = vulkanSamplerAddressMode(info.AddressModeW);
		createInfo.minLod = info.MinLod;
		createInfo.maxLod = (info.MaxLod > VK_LOD_CLAMP_NONE) ? VK_LOD_CLAMP_NONE : info.MaxLod;
		createInfo.maxAnisotropy = anisotropic ? ((info.MaxAnisotropy > anisotropyLimit) ? anisotropyLimit : info.MaxAnisotropy) : 1.0f;
		createInfo.anisotropyEnable = anisotropic ? VK_TRUE : VK_FALSE;
		createInfo.compareEnable = info.ComparisonEnabled ? VK_TRUE : VK_FALSE;
		createInfo.compareOp = info.ComparisonEnabled ? vulkanCompareOp(info.Comparison) : VK_COMPARE_OP_NEVER;

		VkSampler vkSampler = VK_NULL_HANDLE;
		const VkResult vkResult = vkCreateSampler(m_Device, &createInfo, nullptr, &vkSampler);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		Sampler* createdSampler = new Sampler(*this, vkSampler);

		*sampler = Resource<ISampler>(createdSampler);

		return {};
	}

	Status Device::createQueryPool(
		const QueryPoolCreateInfo& info,
		Resource<IQueryPool>* queryPool)
	{
		if (queryPool == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		SPALL_TRY(validateQueryPoolCreateInfo(info));

		if (not m_Limits.SupportsTimestampQueries)
		{
			return ERR_UNSUPPORTED_USAGE;
		}

		VkQueryPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		poolCreateInfo.queryCount = info.TimestampCount;

		VkQueryPool createdPool = VK_NULL_HANDLE;
		const VkResult vkResult = vkCreateQueryPool(m_Device, &poolCreateInfo, nullptr, &createdPool);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		QueryPool* createdQueryPool = new QueryPool(
			*this,
			QueryPoolInfo {info.TimestampCount, info.DebugName},
			createdPool);

		*queryPool = Resource<IQueryPool>(createdQueryPool);

		return {};
	}

	Status Device::readTimestamps(
		IQueryPool& queryPool,
		std::uint32_t firstQuery,
		std::span<std::uint64_t> nanoseconds)
	{
		QueryPool* vkQueryPool = backendCast<QueryPool>(queryPool);

		if ((vkQueryPool == nullptr) or (vkQueryPool->m_Device.get() != this))
		{
			return ERR_INVALID_RESOURCE_TYPE;
		}

		SPALL_TRY(validateTimestampRead(vkQueryPool->m_Info, firstQuery, nanoseconds.size()));

		const std::uint32_t queryCount = static_cast<std::uint32_t>(nanoseconds.size());
		std::vector<std::uint64_t> ticks(queryCount);

		const VkResult vkResult = vkGetQueryPoolResults(
			m_Device,
			vkQueryPool->m_QueryPool,
			firstQuery,
			queryCount,
			ticks.size() * sizeof(std::uint64_t),
			ticks.data(),
			sizeof(std::uint64_t),
			VK_QUERY_RESULT_64_BIT);

		if (vkResult == VK_NOT_READY)
		{
			return ERR_NOT_READY;
		}

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		const std::uint64_t validBitsMask = (m_TimestampValidBits >= 64)
			? (std::numeric_limits<std::uint64_t>::max)()
			: ((std::uint64_t {1} << m_TimestampValidBits) - std::uint64_t {1});

		for (std::uint32_t index = 0; index < queryCount; ++index)
		{
			nanoseconds[index] = static_cast<std::uint64_t>(
				static_cast<double>(ticks[index] & validBitsMask) * static_cast<double>(m_TimestampPeriod));
		}

		return {};
	}

	Status Device::createAccelerationStructure(
		const AccelerationStructureCreateInfo& info,
		Resource<IAccelerationStructure>* accelerationStructure)
	{
		if (accelerationStructure == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		if (not m_RayTracingEnabled)
		{
			return ERR_UNSUPPORTED;
		}

		SPALL_TRY(validateAccelerationStructureCreateInfo(info));

		std::vector<Resource<Buffer>> inputBuffers;
		std::vector<VkAccelerationStructureGeometryKHR> geometries;
		std::vector<std::uint32_t> primitiveCounts;
		Resource<Buffer> instanceBuffer;

		geometries.reserve(info.Geometries.size() + 1);
		primitiveCounts.reserve(info.Geometries.size() + 1);

		const auto resolveInput = [this, &inputBuffers](IBuffer* buffer, Buffer** resolved) -> Status
		{
			Buffer* backendBuffer = backendCast<Buffer>(buffer);

			if ((backendBuffer == nullptr) or (backendBuffer->m_Device.get() != this))
			{
				return ERR_INVALID_RESOURCE_TYPE;
			}

			if ((backendBuffer->m_Info.Usage & BufferUsageFlags::AccelerationStructureInput) == BufferUsageFlags::None)
			{
				return ERR_INVALID_USAGE_FLAGS;
			}

			bool retained = false;

			for (const Resource<Buffer>& existing : inputBuffers)
			{
				if (existing.get() == backendBuffer)
				{
					retained = true;
					break;
				}
			}

			if (not retained)
			{
				inputBuffers.push_back(Resource<Buffer>(backendBuffer));
			}

			*resolved = backendBuffer;

			return {};
		};

		const auto bufferAddress = [this](const Buffer& buffer) -> VkDeviceAddress
		{
			VkBufferDeviceAddressInfo addressInfo = {};
			addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			addressInfo.buffer = buffer.m_Buffer;

			return m_GetBufferDeviceAddress(m_Device, &addressInfo);
		};

		for (const AccelerationStructureGeometry& geometry : info.Geometries)
		{
			if (geometry.Type == AccelerationStructureGeometryType::Aabbs)
			{
				Buffer* aabbBuffer = nullptr;
				SPALL_TRY(resolveInput(geometry.AabbBuffer, &aabbBuffer));

				VkAccelerationStructureGeometryKHR aabbGeometry = {};
				aabbGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
				aabbGeometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
				aabbGeometry.flags = vulkanAccelerationStructureGeometryFlags(geometry.Flags);
				aabbGeometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
				aabbGeometry.geometry.aabbs.data.deviceAddress = bufferAddress(*aabbBuffer) + geometry.AabbOffset;
				aabbGeometry.geometry.aabbs.stride = geometry.AabbStride;

				geometries.push_back(aabbGeometry);
				primitiveCounts.push_back(geometry.AabbCount);

				continue;
			}

			if (not isSupportedAccelerationStructureVertexFormat(geometry.VertexFormat))
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			const std::optional<VkFormat> vertexFormat = toVkFormat(geometry.VertexFormat);

			if (not vertexFormat.has_value())
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			Buffer* vertexBuffer = nullptr;
			SPALL_TRY(resolveInput(geometry.VertexBuffer, &vertexBuffer));

			VkAccelerationStructureGeometryKHR nativeGeometry = {};
			nativeGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			nativeGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			nativeGeometry.flags = vulkanAccelerationStructureGeometryFlags(geometry.Flags);
			nativeGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			nativeGeometry.geometry.triangles.vertexFormat = vertexFormat.value();
			nativeGeometry.geometry.triangles.vertexData.deviceAddress = bufferAddress(*vertexBuffer) + geometry.VertexOffset;
			nativeGeometry.geometry.triangles.vertexStride = geometry.VertexStride;
			nativeGeometry.geometry.triangles.maxVertex = geometry.VertexCount - 1;
			nativeGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

			std::uint32_t primitiveCount = geometry.VertexCount / 3;

			if (geometry.IndexBuffer != nullptr)
			{
				Buffer* indexBuffer = nullptr;
				SPALL_TRY(resolveInput(geometry.IndexBuffer, &indexBuffer));

				nativeGeometry.geometry.triangles.indexType = vulkanIndexType(geometry.IndexFormat);
				nativeGeometry.geometry.triangles.indexData.deviceAddress = bufferAddress(*indexBuffer) + geometry.IndexOffset;

				primitiveCount = geometry.IndexCount / 3;
			}

			if (geometry.TransformBuffer != nullptr)
			{
				Buffer* transformBuffer = nullptr;
				SPALL_TRY(resolveInput(geometry.TransformBuffer, &transformBuffer));

				nativeGeometry.geometry.triangles.transformData.deviceAddress = bufferAddress(*transformBuffer) + geometry.TransformOffset;
			}

			geometries.push_back(nativeGeometry);
			primitiveCounts.push_back(primitiveCount);
		}

		if (info.InstanceBuffer != nullptr)
		{
			Buffer* resolved = nullptr;
			SPALL_TRY(resolveInput(info.InstanceBuffer, &resolved));

			instanceBuffer = Resource<Buffer>(resolved);
		}

		if (info.Type == AccelerationStructureType::TopLevel)
		{
			VkAccelerationStructureGeometryKHR nativeGeometry = {};
			nativeGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			nativeGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			nativeGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			nativeGeometry.geometry.instances.arrayOfPointers = VK_FALSE;

			if (instanceBuffer)
			{
				nativeGeometry.geometry.instances.data.deviceAddress = bufferAddress(*instanceBuffer) + info.InstanceBufferOffset;
			}

			geometries.push_back(nativeGeometry);
			primitiveCounts.push_back(info.InstanceCount);
		}

		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {};
		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.type = vulkanAccelerationStructureType(info.Type);
		buildGeometryInfo.flags = vulkanAccelerationStructureBuildFlags(info.Flags);
		buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo.geometryCount = static_cast<std::uint32_t>(geometries.size());
		buildGeometryInfo.pGeometries = geometries.data();

		VkAccelerationStructureBuildSizesInfoKHR sizes = {};
		sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		m_GetAccelerationStructureBuildSizes(
			m_Device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&buildGeometryInfo,
			primitiveCounts.data(),
			&sizes);

		if ((sizes.accelerationStructureSize == 0) or (sizes.buildScratchSize == 0))
		{
			return ERR_BACKEND_FAILURE;
		}

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkBufferCreateInfo storeCreateInfo = {};
		storeCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		storeCreateInfo.size = sizes.accelerationStructureSize;
		storeCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		storeCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer storeBuffer = VK_NULL_HANDLE;
		VmaAllocation storeAllocation = VK_NULL_HANDLE;
		VkResult vkResult = vmaCreateBuffer(m_Allocator, &storeCreateInfo, &allocationCreateInfo, &storeBuffer, &storeAllocation, nullptr);

		if (vkResult != VK_SUCCESS)
		{
			return mapVulkanStatus(vkResult);
		}

		VkBufferCreateInfo scratchCreateInfo = {};
		scratchCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		scratchCreateInfo.size = (std::max)(sizes.buildScratchSize, sizes.updateScratchSize);
		scratchCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		scratchCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer scratchBuffer = VK_NULL_HANDLE;
		VmaAllocation scratchAllocation = VK_NULL_HANDLE;
		vkResult = vmaCreateBufferWithAlignment(
			m_Allocator,
			&scratchCreateInfo,
			&allocationCreateInfo,
			(std::max)(static_cast<VkDeviceSize>(m_ScratchOffsetAlignment), VkDeviceSize {1}),
			&scratchBuffer,
			&scratchAllocation,
			nullptr);

		if (vkResult != VK_SUCCESS)
		{
			vmaDestroyBuffer(m_Allocator, storeBuffer, storeAllocation);

			return mapVulkanStatus(vkResult);
		}

		VkAccelerationStructureCreateInfoKHR structureCreateInfo = {};
		structureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		structureCreateInfo.buffer = storeBuffer;
		structureCreateInfo.offset = 0;
		structureCreateInfo.size = sizes.accelerationStructureSize;
		structureCreateInfo.type = vulkanAccelerationStructureType(info.Type);

		VkAccelerationStructureKHR structure = VK_NULL_HANDLE;
		vkResult = m_CreateAccelerationStructure(m_Device, &structureCreateInfo, nullptr, &structure);

		if (vkResult != VK_SUCCESS)
		{
			vmaDestroyBuffer(m_Allocator, scratchBuffer, scratchAllocation);
			vmaDestroyBuffer(m_Allocator, storeBuffer, storeAllocation);

			return mapVulkanStatus(vkResult);
		}

		VkBufferDeviceAddressInfo scratchAddressInfo = {};
		scratchAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchAddressInfo.buffer = scratchBuffer;

		const VkDeviceAddress scratchAddress = m_GetBufferDeviceAddress(m_Device, &scratchAddressInfo);

		const bool allowUpdate = ((info.Flags & AccelerationStructureBuildFlags::AllowUpdate) != AccelerationStructureBuildFlags::None);

		AccelerationStructureInfo structureInfo = {};
		structureInfo.Type = info.Type;
		structureInfo.Flags = info.Flags;
		structureInfo.Size = sizes.accelerationStructureSize;
		structureInfo.BuildScratchSize = sizes.buildScratchSize;
		structureInfo.UpdateScratchSize = allowUpdate ? sizes.updateScratchSize : 0;
		structureInfo.GeometryCount = static_cast<std::uint32_t>(info.Geometries.size());
		structureInfo.InstanceCount = info.InstanceCount;
		structureInfo.DebugName = info.DebugName;

		VkQueryPool compactedSizeQueryPool = VK_NULL_HANDLE;

		if (hasAnyFlag(info.Flags, AccelerationStructureBuildFlags::AllowCompaction))
		{
			VkQueryPoolCreateInfo queryPoolCreateInfo = {};
			queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			queryPoolCreateInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
			queryPoolCreateInfo.queryCount = 1;

			vkResult = vkCreateQueryPool(m_Device, &queryPoolCreateInfo, nullptr, &compactedSizeQueryPool);

			if (vkResult != VK_SUCCESS)
			{
				m_DestroyAccelerationStructure(m_Device, structure, nullptr);
				vmaDestroyBuffer(m_Allocator, scratchBuffer, scratchAllocation);
				vmaDestroyBuffer(m_Allocator, storeBuffer, storeAllocation);

				return mapVulkanStatus(vkResult);
			}
		}

		AccelerationStructure* created = new AccelerationStructure(
			*this,
			structureInfo,
			structure,
			storeBuffer,
			storeAllocation,
			scratchBuffer,
			scratchAllocation,
			scratchAddress,
			std::move(inputBuffers),
			std::move(geometries),
			std::move(primitiveCounts),
			std::move(instanceBuffer),
			info.InstanceBufferOffset);

		created->m_CompactedSizeQueryPool = compactedSizeQueryPool;

		*accelerationStructure = Resource<IAccelerationStructure>(created);

		return {};
	}

} // namespace spall::vk
