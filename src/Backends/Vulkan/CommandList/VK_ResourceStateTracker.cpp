// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/Vulkan/CommandList/VK_ResourceStateTracker.h>

#include <src/Backends/Vulkan/Common/VK_EnumMappings.h>
#include <src/Backends/Vulkan/Common/VK_PresentSync.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_Buffer.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_BufferState.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Validation/Common/TextureValidation.h>

#include <optional>

namespace spall::vk
{
	void ResourceStateTracker::appendImageBarrier(
		std::vector<VkImageMemoryBarrier>& barriers,
		VkImage image,
		VkImageAspectFlags aspectMask,
		const TextureStateInfo& before,
		const TextureStateInfo& after,
		std::uint32_t mipLevel,
		std::uint32_t baseArrayLayer,
		std::uint32_t arrayLayers)
	{
		if (not barriers.empty())
		{
			VkImageMemoryBarrier& last = barriers.back();
			const bool sameTransition = (last.image == image) and (last.subresourceRange.aspectMask == aspectMask) and
				(last.oldLayout == before.layout) and (last.newLayout == after.layout) and
				(last.srcAccessMask == before.access) and (last.dstAccessMask == after.access);

			if (sameTransition)
			{
				VkImageSubresourceRange& range = last.subresourceRange;

				if ((range.levelCount == 1) and (range.baseMipLevel == mipLevel) and
					((range.baseArrayLayer + range.layerCount) == baseArrayLayer))
				{
					range.layerCount += arrayLayers;

					return;
				}

				if ((range.baseArrayLayer == baseArrayLayer) and (range.layerCount == arrayLayers) and
					((range.baseMipLevel + range.levelCount) == mipLevel))
				{
					++range.levelCount;

					return;
				}
			}
		}

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = before.access;
		barrier.dstAccessMask = after.access;
		barrier.oldLayout = before.layout;
		barrier.newLayout = after.layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = aspectMask;
		barrier.subresourceRange.baseMipLevel = mipLevel;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
		barrier.subresourceRange.layerCount = arrayLayers;

		barriers.push_back(barrier);
	}

	ResourceStateTracker::ResourceStateTracker(
		VkCommandBuffer commandBuffer)
		: m_CommandBuffer(commandBuffer)
	{
	}

	ResourceStateTracker::TextureState& ResourceStateTracker::trackedTextureState(
		Texture& texture)
	{
		auto trackedState = m_TextureStates.find(&texture);

		if (trackedState == m_TextureStates.end())
		{
			trackedState = m_TextureStates.emplace(&texture, TextureState {}).first;
			trackedState->second.Subresources.resize(texture.m_SubresourceStates.size());
		}

		return trackedState->second;
	}

	void ResourceStateTracker::appendTextureBarrier(
		Texture& texture,
		ResourceStateFlags stateBefore,
		ResourceStateFlags stateAfter,
		std::uint32_t mipLevel,
		std::uint32_t arrayLayer)
	{
		if (not m_TextureBarriers.empty())
		{
			TextureBarrier& last = m_TextureBarriers.back();

			if ((last.Resource == &texture) and (last.StateBefore == stateBefore) and (last.StateAfter == stateAfter) and
				(last.MipLevel == mipLevel) and ((last.BaseArrayLayer + last.ArrayLayers) == arrayLayer))
			{
				++last.ArrayLayers;

				return;
			}
		}

		m_TextureBarriers.push_back(TextureBarrier {&texture, stateBefore, stateAfter, mipLevel, arrayLayer, 1});
	}

	Status ResourceStateTracker::beginTrackingTextureState(
		Texture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		const std::optional<TextureStateInfo> stateInfo = vulkanTextureState(state);

		if ((texture.m_PermanentState != ResourceStateFlags::Unknown) or
			(not stateInfo.has_value()) or (stateInfo->layout == VK_IMAGE_LAYOUT_UNDEFINED))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		Status error = validateTextureSubresourceRange(texture.m_Info, subresources);

		if (error != SUCCESS)
		{
			return error;
		}

		const TextureSubresourceRange range = resolveTextureSubresourceRange(texture.m_Info, subresources);
		TextureState& trackedState = trackedTextureState(texture);

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);

				if (trackedState.Subresources[subresource].CurrentState != ResourceStateFlags::Unknown)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}
			}
		}

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);
				const bool uninitialized = not texture.m_SubresourceStates[subresource].Initialized;

				trackedState.Subresources[subresource] = TextureSubresourceState {
					state,
					uninitialized ? state : ResourceStateFlags::Unknown,
					uninitialized};
			}
		}

		return {};
	}

	Status ResourceStateTracker::beginTrackingBufferState(
		Buffer& buffer,
		ResourceStateFlags state)
	{
		if ((buffer.m_PermanentState != ResourceStateFlags::Unknown) or
			(m_BufferStates.find(&buffer) != m_BufferStates.end()) or (not vulkanBufferState(state).has_value()))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		m_BufferStates[&buffer] = BufferState {.CurrentState = state};
		return {};
	}

	ResourceStateFlags ResourceStateTracker::globalSubresourceState(
		const Texture& texture,
		std::uint32_t subresource)
	{
		if (texture.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return texture.m_PermanentState;
		}

		const Texture::SubresourceState& state = texture.m_SubresourceStates[subresource];

		if (texture.m_IsSwapChainTexture and (not state.Initialized))
		{
			return ResourceStateFlags::Unknown;
		}

		return state.State;
	}

	ResourceStateFlags ResourceStateTracker::globalBufferState(
		const Buffer& buffer)
	{
		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return buffer.m_PermanentState;
		}

		return buffer.m_CurrentState;
	}

	ResourceStateFlags ResourceStateTracker::currentTextureState(
		Texture& texture,
		const TextureSubresourceRange& subresources) const
	{
		if (validateTextureSubresourceRange(texture.m_Info, subresources) != SUCCESS)
		{
			return ResourceStateFlags::Unknown;
		}

		const TextureSubresourceRange range = resolveTextureSubresourceRange(texture.m_Info, subresources);
		const auto trackedState = m_TextureStates.find(&texture);
		ResourceStateFlags sharedState = ResourceStateFlags::Unknown;
		bool haveSharedState = false;

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);
				ResourceStateFlags state = (trackedState != m_TextureStates.end())
					? trackedState->second.Subresources[subresource].CurrentState
					: ResourceStateFlags::Unknown;

				if (state == ResourceStateFlags::Unknown)
				{
					state = globalSubresourceState(texture, subresource);
				}

				if (not haveSharedState)
				{
					sharedState = state;
					haveSharedState = true;
				}
				else if (sharedState != state)
				{
					return ResourceStateFlags::Unknown;
				}
			}
		}

		return sharedState;
	}

	Status ResourceStateTracker::validateTextureState(
		Texture& texture,
		ResourceStateFlags requiredState,
		const TextureSubresourceRange& subresources)
	{
		Status error = validateTextureSubresourceRange(texture.m_Info, subresources);

		if (error != SUCCESS)
		{
			return error;
		}

		if (texture.m_PermanentState != ResourceStateFlags::Unknown)
		{
			const std::uint32_t permanentState = static_cast<std::uint32_t>(texture.m_PermanentState);
			const std::uint32_t required = static_cast<std::uint32_t>(requiredState);

			if ((permanentState & required) != required)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			return {};
		}

		const TextureSubresourceRange range = resolveTextureSubresourceRange(texture.m_Info, subresources);

		if (currentTextureState(texture, range) != requiredState)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (requiredState == ResourceStateFlags::Unknown)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		TextureState& trackedState = trackedTextureState(texture);

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);
				TextureSubresourceState& state = trackedState.Subresources[subresource];

				if (state.CurrentState == ResourceStateFlags::Unknown)
				{
					state.CurrentState = requiredState;
				}

				state.InitializesState = state.InitializesState or (not texture.m_SubresourceStates[subresource].Initialized);
			}
		}

		return {};
	}

	Status ResourceStateTracker::requireTextureState(
		Texture& texture,
		ResourceStateFlags newState,
		const TextureSubresourceRange& subresources)
	{
		if (texture.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return validateTextureState(texture, newState, subresources);
		}

		const auto pendingPermanentState = m_TextureStates.find(&texture);

		if ((pendingPermanentState != m_TextureStates.end()) and pendingPermanentState->second.PermanentTransition)
		{
			return validateTextureState(texture, newState, subresources);
		}

		const std::optional<TextureStateInfo> target = vulkanTextureState(newState);

		if ((not target.has_value()) or (target->layout == VK_IMAGE_LAYOUT_UNDEFINED))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		Status error = validateTextureSubresourceRange(texture.m_Info, subresources);

		if (error != SUCCESS)
		{
			return error;
		}

		const TextureSubresourceRange range = resolveTextureSubresourceRange(texture.m_Info, subresources);
		TextureState& trackedState = trackedTextureState(texture);

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);
				TextureSubresourceState& state = trackedState.Subresources[subresource];
				const bool uninitialized = not texture.m_SubresourceStates[subresource].Initialized;

				if (state.CurrentState == ResourceStateFlags::Unknown)
				{
					state.CurrentState = newState;
					state.EntryState = newState;
					state.InitializesState = state.InitializesState or uninitialized;

					continue;
				}

				const bool transitionNecessary = state.CurrentState != newState;
				const bool unorderedAccessBarrierNecessary = newState == ResourceStateFlags::UnorderedAccess;

				if (transitionNecessary or unorderedAccessBarrierNecessary)
				{
					appendTextureBarrier(texture, state.CurrentState, newState, mipLevel, arrayLayer);
					state.CurrentState = newState;
				}

				state.InitializesState = state.InitializesState or uninitialized;
			}
		}

		return {};
	}

	ResourceStateFlags ResourceStateTracker::currentBufferState(
		Buffer& buffer) const
	{
		const auto trackedState = m_BufferStates.find(&buffer);

		if (trackedState != m_BufferStates.end())
		{
			return trackedState->second.CurrentState;
		}

		return globalBufferState(buffer);
	}

	Status ResourceStateTracker::validateBufferState(
		Buffer& buffer,
		ResourceStateFlags requiredState)
	{
		const ResourceStateFlags trackedState = currentBufferState(buffer);
		const std::uint32_t currentState = static_cast<std::uint32_t>(trackedState);
		const std::uint32_t required = static_cast<std::uint32_t>(requiredState);

		if ((required == 0) or (trackedState == ResourceStateFlags::Unknown) or ((currentState & required) != required))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return {};
		}

		if (m_BufferStates.find(&buffer) == m_BufferStates.end())
		{
			m_BufferStates.emplace(&buffer, BufferState {.CurrentState = trackedState});
		}

		return {};
	}

	Status ResourceStateTracker::requireBufferState(
		Buffer& buffer,
		ResourceStateFlags newState)
	{
		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return validateBufferState(buffer, newState);
		}

		const auto pendingPermanentState = m_BufferStates.find(&buffer);

		if ((pendingPermanentState != m_BufferStates.end()) and pendingPermanentState->second.PermanentTransition)
		{
			return validateBufferState(buffer, newState);
		}

		const std::optional<BufferStateInfo> target = vulkanBufferState(newState);

		if (not target.has_value())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		auto trackedState = m_BufferStates.find(&buffer);

		if (trackedState == m_BufferStates.end())
		{
			m_BufferStates.emplace(&buffer, BufferState {.CurrentState = newState, .EntryState = newState, .HasEntryState = true});

			return {};
		}

		const ResourceStateFlags currentState = trackedState->second.CurrentState;
		const bool transitionNecessary = currentState != newState;
		const bool uavNecessary = (newState & ResourceStateFlags::UnorderedAccess) != ResourceStateFlags::Unknown;

		if (transitionNecessary)
		{
			for (BufferBarrier& barrier : m_BufferBarriers)
			{
				if (barrier.Resource == &buffer)
				{
					barrier.StateAfter |= newState;
					trackedState->second.CurrentState = barrier.StateAfter;
					return {};
				}
			}
		}

		if (transitionNecessary or uavNecessary)
		{
			m_BufferBarriers.push_back(BufferBarrier {&buffer, currentState, newState});
		}

		trackedState->second.CurrentState = newState;

		return {};
	}

	Status ResourceStateTracker::setPermanentTextureState(
		Texture& texture,
		ResourceStateFlags state)
	{
		if (texture.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return (texture.m_PermanentState == state)
				? SUCCESS
				: ERR_INVALID_RESOURCE_STATE;
		}

		auto trackedState = m_TextureStates.find(&texture);

		if ((trackedState != m_TextureStates.end()) and trackedState->second.PermanentTransition)
		{
			return (currentTextureState(texture) == state)
				? SUCCESS
				: ERR_INVALID_RESOURCE_STATE;
		}

		Status error = requireTextureState(texture, state);

		if (error != SUCCESS)
		{
			return error;
		}

		trackedState = m_TextureStates.find(&texture);
		trackedState->second.PermanentTransition = true;
		m_PermanentTextureStates.emplace_back(&texture, state);

		return {};
	}

	Status ResourceStateTracker::setPermanentBufferState(
		Buffer& buffer,
		ResourceStateFlags state)
	{
		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return (buffer.m_PermanentState == state)
				? SUCCESS
				: ERR_INVALID_RESOURCE_STATE;
		}

		auto trackedState = m_BufferStates.find(&buffer);

		if ((trackedState != m_BufferStates.end()) and trackedState->second.PermanentTransition)
		{
			return (trackedState->second.CurrentState == state)
				? SUCCESS
				: ERR_INVALID_RESOURCE_STATE;
		}

		Status error = requireBufferState(buffer, state);

		if (error != SUCCESS)
		{
			return error;
		}

		trackedState = m_BufferStates.find(&buffer);
		trackedState->second.PermanentTransition = true;
		m_PermanentBufferStates.emplace_back(&buffer, state);

		return {};
	}

	Status ResourceStateTracker::keepInitialStates()
	{
		for (auto& trackedState : m_TextureStates)
		{
			Texture& texture = *trackedState.first;

			if ((not texture.m_Info.KeepInitialState) or
				(texture.m_PermanentState != ResourceStateFlags::Unknown) or
				trackedState.second.PermanentTransition)
			{
				continue;
			}

			for (std::uint32_t mipLevel = 0; mipLevel < texture.m_Info.MipLevels; ++mipLevel)
			{
				for (std::uint32_t arrayLayer = 0; arrayLayer < texture.m_Info.ArrayLayers; ++arrayLayer)
				{
					const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);

					if (trackedState.second.Subresources[subresource].CurrentState == ResourceStateFlags::Unknown)
					{
						continue;
					}

					Status error = requireTextureState(
						texture,
						texture.m_Info.InitialState,
						TextureSubresourceRange {mipLevel, 1, arrayLayer, 1});

					if (error != SUCCESS)
					{
						return error;
					}
				}
			}
		}

		for (auto& trackedState : m_BufferStates)
		{
			Buffer& buffer = *trackedState.first;

			if (buffer.m_Info.KeepInitialState and
				(buffer.m_PermanentState == ResourceStateFlags::Unknown) and
				(not trackedState.second.PermanentTransition))
			{
				Status error = requireBufferState(buffer, buffer.m_Info.InitialState);

				if (error != SUCCESS)
				{
					return error;
				}
			}
		}

		return {};
	}

	Status ResourceStateTracker::commitBarriers()
	{
		std::vector<VkImageMemoryBarrier> imageBarriers;
		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		VkPipelineStageFlags sourceStages = 0;
		VkPipelineStageFlags destinationStages = 0;

		imageBarriers.reserve(m_TextureBarriers.size());
		bufferBarriers.reserve(m_BufferBarriers.size());

		for (const TextureBarrier& pendingBarrier : m_TextureBarriers)
		{
			const std::optional<TextureStateInfo> before = vulkanTextureState(pendingBarrier.StateBefore);
			const std::optional<TextureStateInfo> after = vulkanTextureState(pendingBarrier.StateAfter);
			const bool unsupportedSource = (not before.has_value()) or (before->layout == VK_IMAGE_LAYOUT_UNDEFINED);
			const bool unsupportedTarget = (not after.has_value()) or (after->layout == VK_IMAGE_LAYOUT_UNDEFINED);

			if (unsupportedSource or unsupportedTarget)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			appendImageBarrier(
				imageBarriers,
				pendingBarrier.Resource->m_Image,
				pendingBarrier.Resource->m_AspectMask,
				before.value(),
				after.value(),
				pendingBarrier.MipLevel,
				pendingBarrier.BaseArrayLayer,
				pendingBarrier.ArrayLayers);

			sourceStages |= before->stage;
			destinationStages |= after->stage;

			if (pendingBarrier.Resource->m_IsSwapChainTexture)
			{
				sourceStages |= SwapChainAcquireWaitStages;
			}
		}

		for (const BufferBarrier& pendingBarrier : m_BufferBarriers)
		{
			const std::optional<BufferStateInfo> before = vulkanBufferState(pendingBarrier.StateBefore);
			const std::optional<BufferStateInfo> after = vulkanBufferState(pendingBarrier.StateAfter);

			if ((not before.has_value()) or (not after.has_value()))
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask = before->access;
			barrier.dstAccessMask = after->access;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.buffer = pendingBarrier.Resource->m_Buffer;
			barrier.offset = 0;
			barrier.size = VK_WHOLE_SIZE;

			bufferBarriers.push_back(barrier);
			sourceStages |= before->stage;
			destinationStages |= after->stage;
		}

		if ((not imageBarriers.empty()) or (not bufferBarriers.empty()))
		{
			vkCmdPipelineBarrier(
				m_CommandBuffer,
				sourceStages != 0 ? sourceStages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				destinationStages != 0 ? destinationStages : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0,
				nullptr,
				static_cast<std::uint32_t>(bufferBarriers.size()),
				bufferBarriers.data(),
				static_cast<std::uint32_t>(imageBarriers.size()),
				imageBarriers.data());
		}

		m_TextureBarriers.clear();
		m_BufferBarriers.clear();

		return {};
	}

	bool ResourceStateTracker::hasPendingBarriers() const
	{
		return (not m_TextureBarriers.empty()) or (not m_BufferBarriers.empty());
	}

	Status ResourceStateTracker::recordEntryBarriers(
		VkCommandBuffer commandBuffer,
		bool* recordedBarriers) const
	{
		*recordedBarriers = false;

		std::vector<VkImageMemoryBarrier> imageBarriers;
		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		VkPipelineStageFlags sourceStages = 0;
		VkPipelineStageFlags destinationStages = 0;

		for (const auto& trackedState : m_TextureStates)
		{
			Texture& texture = *trackedState.first;

			for (std::uint32_t mipLevel = 0; mipLevel < texture.m_Info.MipLevels; ++mipLevel)
			{
				for (std::uint32_t arrayLayer = 0; arrayLayer < texture.m_Info.ArrayLayers; ++arrayLayer)
				{
					const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);
					const ResourceStateFlags entryState = trackedState.second.Subresources[subresource].EntryState;

					if (entryState == ResourceStateFlags::Unknown)
					{
						continue;
					}

					const bool uninitialized = not texture.m_SubresourceStates[subresource].Initialized;
					const ResourceStateFlags priorState = uninitialized
						? ResourceStateFlags::Unknown
						: globalSubresourceState(texture, subresource);

					if ((not uninitialized) and (priorState == entryState))
					{
						continue;
					}

					const std::optional<TextureStateInfo> before = vulkanTextureState(priorState);
					const std::optional<TextureStateInfo> after = vulkanTextureState(entryState);
					const bool unsupportedTarget = (not after.has_value()) or (after->layout == VK_IMAGE_LAYOUT_UNDEFINED);

					if ((not before.has_value()) or unsupportedTarget)
					{
						return ERR_INVALID_RESOURCE_STATE;
					}

					appendImageBarrier(
						imageBarriers,
						texture.m_Image,
						texture.m_AspectMask,
						before.value(),
						after.value(),
						mipLevel,
						arrayLayer,
						1);

					sourceStages |= before->stage;
					destinationStages |= after->stage;

					if (texture.m_IsSwapChainTexture)
					{
						sourceStages |= SwapChainAcquireWaitStages;
					}
				}
			}
		}

		for (const auto& trackedState : m_BufferStates)
		{
			if (not trackedState.second.HasEntryState)
			{
				continue;
			}

			Buffer& buffer = *trackedState.first;
			const ResourceStateFlags entryState = trackedState.second.EntryState;
			const ResourceStateFlags priorState = globalBufferState(buffer);
			const bool unorderedAccess = (entryState & ResourceStateFlags::UnorderedAccess) != ResourceStateFlags::Unknown;

			if ((priorState == entryState) and (not unorderedAccess))
			{
				continue;
			}

			const std::optional<BufferStateInfo> before = vulkanBufferState(priorState);
			const std::optional<BufferStateInfo> after = vulkanBufferState(entryState);

			if ((not before.has_value()) or (not after.has_value()))
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask = before->access;
			barrier.dstAccessMask = after->access;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.buffer = buffer.m_Buffer;
			barrier.offset = 0;
			barrier.size = VK_WHOLE_SIZE;

			bufferBarriers.push_back(barrier);
			sourceStages |= before->stage;
			destinationStages |= after->stage;
		}

		if (imageBarriers.empty() and bufferBarriers.empty())
		{
			return {};
		}

		*recordedBarriers = true;

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStages != 0 ? sourceStages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			destinationStages != 0 ? destinationStages : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0,
			nullptr,
			static_cast<std::uint32_t>(bufferBarriers.size()),
			bufferBarriers.data(),
			static_cast<std::uint32_t>(imageBarriers.size()),
			imageBarriers.data());

		return {};
	}

	Status ResourceStateTracker::validateSubmissionStates() const
	{
		for (const auto& trackedState : m_TextureStates)
		{
			const Texture& texture = *trackedState.first;

			if (texture.m_PermanentState != ResourceStateFlags::Unknown)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			if (not texture.m_IsSwapChainTexture)
			{
				continue;
			}

			for (std::size_t subresource = 0; subresource < trackedState.second.Subresources.size(); ++subresource)
			{
				if (trackedState.second.Subresources[subresource].InitializesState and
					texture.m_SubresourceStates[subresource].Initialized)
				{
					return ERR_INVALID_RESOURCE_STATE;
				}
			}
		}

		for (const auto& trackedState : m_BufferStates)
		{
			if (trackedState.first->m_PermanentState != ResourceStateFlags::Unknown)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}
		}

		for (const auto& permanentState : m_PermanentTextureStates)
		{
			if (permanentState.first->m_PermanentState != ResourceStateFlags::Unknown)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}
		}

		for (const auto& permanentState : m_PermanentBufferStates)
		{
			if (permanentState.first->m_PermanentState != ResourceStateFlags::Unknown)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}
		}

		return {};
	}

	void ResourceStateTracker::commandListSubmitted()
	{
		for (const auto& trackedState : m_TextureStates)
		{
			Texture& texture = *trackedState.first;

			for (std::size_t subresource = 0; subresource < trackedState.second.Subresources.size(); ++subresource)
			{
				const TextureSubresourceState& state = trackedState.second.Subresources[subresource];

				if (state.CurrentState == ResourceStateFlags::Unknown)
				{
					continue;
				}

				if (state.InitializesState)
				{
					texture.m_SubresourceStates[subresource].Initialized = true;
				}

				texture.m_SubresourceStates[subresource].State = state.CurrentState;
			}
		}

		for (const auto& trackedState : m_BufferStates)
		{
			trackedState.first->m_CurrentState = trackedState.second.CurrentState;
		}

		for (const auto& permanentState : m_PermanentTextureStates)
		{
			permanentState.first->m_PermanentState = permanentState.second;

			for (Texture::SubresourceState& state : permanentState.first->m_SubresourceStates)
			{
				state.State = permanentState.second;
				state.Initialized = true;
			}
		}

		for (const auto& permanentState : m_PermanentBufferStates)
		{
			permanentState.first->m_PermanentState = permanentState.second;
			permanentState.first->m_CurrentState = permanentState.second;
		}

		m_TextureStates.clear();
		m_BufferStates.clear();
		m_TextureBarriers.clear();
		m_BufferBarriers.clear();
		m_PermanentTextureStates.clear();
		m_PermanentBufferStates.clear();
	}

	void ResourceStateTracker::reset()
	{
		m_TextureStates.clear();
		m_BufferStates.clear();
		m_TextureBarriers.clear();
		m_BufferBarriers.clear();
		m_PermanentTextureStates.clear();
		m_PermanentBufferStates.clear();
	}
} // namespace spall::vk
