// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Resources/Texture/TextureSubresourceRange.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>
#include <src/Backends/Vulkan/Common/VK_TextureStateInfo.h>

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spall::vk
{
	class Buffer;
	class Texture;

	class ResourceStateTracker
	{
	public:
		explicit ResourceStateTracker(VkCommandBuffer commandBuffer);

		Status beginTrackingTextureState(
			Texture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources = {});

		Status beginTrackingBufferState(
			Buffer& buffer,
			ResourceStateFlags state);

		Status requireTextureState(
			Texture& texture,
			ResourceStateFlags newState,
			const TextureSubresourceRange& subresources = {});

		Status requireBufferState(
			Buffer& buffer,
			ResourceStateFlags newState);

		Status validateTextureState(
			Texture& texture,
			ResourceStateFlags requiredState,
			const TextureSubresourceRange& subresources = {});

		Status validateBufferState(
			Buffer& buffer,
			ResourceStateFlags requiredState);

		ResourceStateFlags currentTextureState(
			Texture& texture,
			const TextureSubresourceRange& subresources = {}) const;

		ResourceStateFlags currentBufferState(Buffer& buffer) const;

		Status setPermanentTextureState(
			Texture& texture,
			ResourceStateFlags state);

		Status setPermanentBufferState(
			Buffer& buffer,
			ResourceStateFlags state);

		Status keepInitialStates(void);

		Status commitBarriers(void);
		bool hasPendingBarriers(void) const;

		Status recordEntryBarriers(
			VkCommandBuffer commandBuffer,
			bool* recordedBarriers) const;

		Status validateSubmissionStates(void) const;
		void commandListSubmitted(void);

		void reset(void);

	private:
		static void appendImageBarrier(
			std::vector<VkImageMemoryBarrier>& barriers,
			VkImage image,
			VkImageAspectFlags aspectMask,
			const TextureStateInfo& before,
			const TextureStateInfo& after,
			std::uint32_t mipLevel,
			std::uint32_t baseArrayLayer,
			std::uint32_t arrayLayers);

		static ResourceStateFlags globalSubresourceState(
			const Texture& texture,
			std::uint32_t subresource);

		static ResourceStateFlags globalBufferState(const Buffer& buffer);

		struct TextureSubresourceState
		{
			ResourceStateFlags CurrentState = ResourceStateFlags::Unknown;
			ResourceStateFlags EntryState = ResourceStateFlags::Unknown;
			bool InitializesState = false;
		};

		struct TextureState
		{
			std::vector<TextureSubresourceState> Subresources;
			bool PermanentTransition = false;
		};

		TextureState& trackedTextureState(Texture& texture);

		void appendTextureBarrier(
			Texture& texture,
			ResourceStateFlags stateBefore,
			ResourceStateFlags stateAfter,
			std::uint32_t mipLevel,
			std::uint32_t arrayLayer);

		struct BufferState
		{
			ResourceStateFlags CurrentState = ResourceStateFlags::Unknown;
			bool PermanentTransition = false;
			ResourceStateFlags EntryState = ResourceStateFlags::Unknown;
			bool HasEntryState = false;
		};

		struct TextureBarrier
		{
			Texture* Resource = nullptr;
			ResourceStateFlags StateBefore = ResourceStateFlags::Unknown;
			ResourceStateFlags StateAfter = ResourceStateFlags::Unknown;
			std::uint32_t MipLevel = 0;
			std::uint32_t BaseArrayLayer = 0;
			std::uint32_t ArrayLayers = 1;
		};

		struct BufferBarrier
		{
			Buffer* Resource = nullptr;
			ResourceStateFlags StateBefore = ResourceStateFlags::Unknown;
			ResourceStateFlags StateAfter = ResourceStateFlags::Unknown;
		};

		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

		std::unordered_map<Texture*, TextureState> m_TextureStates;
		std::unordered_map<Buffer*, BufferState> m_BufferStates;

		std::vector<TextureBarrier> m_TextureBarriers;
		std::vector<BufferBarrier> m_BufferBarriers;

		std::vector<std::pair<Texture*, ResourceStateFlags>> m_PermanentTextureStates;
		std::vector<std::pair<Buffer*, ResourceStateFlags>> m_PermanentBufferStates;
	};
} // namespace spall::vk
