// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Backends/D3D12/CommandList/D3D12_ResourceStateTracker.h>

#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/Common/Resources/D3D12_CopyLayout.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_HeapMappings.h>
#include <src/Backends/D3D12/Common/Mappings/D3D12_ResourceStateMappings.h>
#include <src/Backends/D3D12/Resources/Buffer/D3D12_Buffer.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Validation/Common/ResourceStateValidation.h>
#include <src/Validation/Common/TextureValidation.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>

namespace spall::d3d12
{
	void ResourceStateTracker::setCommandList(
		ID3D12GraphicsCommandList* commandList)
	{
		m_CommandList = commandList;
	}

	ResourceStateFlags ResourceStateTracker::globalSubresourceState(
		const Texture& texture,
		std::uint32_t subresource)
	{
		if (texture.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return texture.m_PermanentState;
		}

		return texture.m_SubresourceStates[subresource];
	}

	ResourceStateFlags ResourceStateTracker::globalBufferState(
		const Buffer& buffer)
	{
		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return buffer.m_PermanentState;
		}

		return buffer.m_State;
	}

	ResourceStateTracker::TextureState& ResourceStateTracker::trackedTextureState(
		Texture& texture)
	{
		auto trackedState = m_TextureStates.find(&texture);

		if (trackedState == m_TextureStates.end())
		{
			TextureState created = {};
			created.Subresources.reserve(texture.m_SubresourceStates.size());

			for (std::size_t subresource = 0; subresource < texture.m_SubresourceStates.size(); ++subresource)
			{
				created.Subresources.push_back(globalSubresourceState(texture, static_cast<std::uint32_t>(subresource)));
			}

			trackedState = m_TextureStates.emplace(&texture, std::move(created)).first;
		}

		return trackedState->second;
	}

	ResourceStateTracker::ResourceState& ResourceStateTracker::trackedBufferState(
		Buffer& buffer)
	{
		auto trackedState = m_BufferStates.find(&buffer);

		if (trackedState == m_BufferStates.end())
		{
			trackedState = m_BufferStates.emplace(&buffer, ResourceState {globalBufferState(buffer), false}).first;
		}

		return trackedState->second;
	}

	void ResourceStateTracker::appendTransition(
		ID3D12Resource& resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = &resource;
		barrier.Transition.Subresource = subresource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		m_PendingBarriers.push_back(barrier);
	}

	Status ResourceStateTracker::beginTrackingTextureState(
		Texture& texture,
		ResourceStateFlags state,
		const TextureSubresourceRange& subresources)
	{
		if ((texture.m_PermanentState != ResourceStateFlags::Unknown) or (state == ResourceStateFlags::Unknown))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateTextureSubresourceRange(texture.m_Info, subresources));

		if (m_TextureStates.find(&texture) != m_TextureStates.end())
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		const TextureSubresourceRange range = resolveTextureSubresourceRange(texture.m_Info, subresources);
		TextureState& trackedState = trackedTextureState(texture);

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				trackedState.Subresources[textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer)] = state;
			}
		}

		return {};
	}

	Status ResourceStateTracker::beginTrackingBufferState(
		Buffer& buffer,
		ResourceStateFlags state)
	{
		if ((buffer.m_PermanentState != ResourceStateFlags::Unknown) or
			(m_BufferStates.find(&buffer) != m_BufferStates.end()) or
			(state == ResourceStateFlags::Unknown))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		m_BufferStates.emplace(&buffer, ResourceState {state, false});

		return {};
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
				const ResourceStateFlags state = (trackedState != m_TextureStates.end())
					? trackedState->second.Subresources[subresource]
					: globalSubresourceState(texture, subresource);

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

	Status ResourceStateTracker::validateTextureState(
		Texture& texture,
		ResourceStateFlags requiredState,
		const TextureSubresourceRange& subresources)
	{
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

		SPALL_TRY(validateTextureSubresourceRange(texture.m_Info, subresources));

		if ((requiredState == ResourceStateFlags::Unknown) or (currentTextureState(texture, subresources) != requiredState))
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		return {};
	}

	Status ResourceStateTracker::validateBufferState(
		Buffer& buffer,
		ResourceStateFlags requiredState)
	{
		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			const std::uint32_t permanentState = static_cast<std::uint32_t>(buffer.m_PermanentState);
			const std::uint32_t required = static_cast<std::uint32_t>(requiredState);

			if ((permanentState & required) != required)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			return {};
		}

		if ((requiredState == ResourceStateFlags::Unknown) or (currentBufferState(buffer) != requiredState))
		{
			return ERR_INVALID_RESOURCE_STATE;
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

		if (newState == ResourceStateFlags::Unknown)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateTextureSubresourceRange(texture.m_Info, subresources));
		SPALL_TRY(validateTextureResourceState(texture.m_Info, newState, texture.m_IsSwapChainTexture));

		const TextureSubresourceRange range = resolveTextureSubresourceRange(texture.m_Info, subresources);
		TextureState& trackedState = trackedTextureState(texture);

		const bool wholeResource = (range.BaseMipLevel == 0) and (range.MipLevels == texture.m_Info.MipLevels) and
			(range.BaseArrayLayer == 0) and (range.ArrayLayers == texture.m_Info.ArrayLayers);

		const D3D12_RESOURCE_STATES stateAfter = resourceState(newState);

		if (wholeResource)
		{
			bool uniformBefore = true;

			for (const ResourceStateFlags subresourceState : trackedState.Subresources)
			{
				if (subresourceState != trackedState.Subresources.front())
				{
					uniformBefore = false;
					break;
				}
			}

			if (uniformBefore)
			{
				const ResourceStateFlags before = trackedState.Subresources.front();

				if (before == newState)
				{
					if (newState == ResourceStateFlags::UnorderedAccess)
					{
						D3D12_RESOURCE_BARRIER barrier = {};
						barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
						barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
						barrier.UAV.pResource = texture.m_Resource.Get();

						m_PendingBarriers.push_back(barrier);
					}

					return {};
				}

				const D3D12_RESOURCE_STATES stateBefore = resourceState(before);

				if (stateBefore != stateAfter)
				{
					appendTransition(
						*texture.m_Resource.Get(),
						stateBefore,
						stateAfter,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				}

				trackedState.Subresources.assign(trackedState.Subresources.size(), newState);

				return {};
			}
		}

		for (std::uint32_t mipLevel = range.BaseMipLevel; mipLevel < (range.BaseMipLevel + range.MipLevels); ++mipLevel)
		{
			for (std::uint32_t arrayLayer = range.BaseArrayLayer; arrayLayer < (range.BaseArrayLayer + range.ArrayLayers); ++arrayLayer)
			{
				const std::uint32_t subresource = textureSubresourceIndex(texture.m_Info, mipLevel, arrayLayer);
				const ResourceStateFlags before = trackedState.Subresources[subresource];

				if (before == newState)
				{
					if (newState == ResourceStateFlags::UnorderedAccess)
					{
						D3D12_RESOURCE_BARRIER barrier = {};
						barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
						barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
						barrier.UAV.pResource = texture.m_Resource.Get();

						m_PendingBarriers.push_back(barrier);
					}

					continue;
				}

				const D3D12_RESOURCE_STATES stateBefore = resourceState(before);

				if (stateBefore != stateAfter)
				{
					appendTransition(
						*texture.m_Resource.Get(),
						stateBefore,
						stateAfter,
						subresourceIndex(texture.m_Info, mipLevel, arrayLayer));
				}

				trackedState.Subresources[subresource] = newState;
			}
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

		if (newState == ResourceStateFlags::Unknown)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateBufferResourceState(buffer.m_Info, newState));

		const D3D12_RESOURCE_STATES stateAfter = resourceState(newState);

		if (isFixedStateHeap(buffer.m_HeapType))
		{
			const D3D12_RESOURCE_STATES pinnedState = fixedHeapState(buffer.m_HeapType);

			if ((stateAfter & ~pinnedState) != 0)
			{
				return ERR_INVALID_RESOURCE_STATE;
			}

			trackedBufferState(buffer).CurrentState = newState;

			return {};
		}

		ResourceState& trackedState = trackedBufferState(buffer);

		if (trackedState.CurrentState == newState)
		{
			if (newState != ResourceStateFlags::UnorderedAccess)
			{
				return {};
			}

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.UAV.pResource = buffer.m_Resource.Get();

			m_PendingBarriers.push_back(barrier);

			return {};
		}

		const D3D12_RESOURCE_STATES stateBefore = resourceState(trackedState.CurrentState);

		if (stateBefore != stateAfter)
		{
			appendTransition(*buffer.m_Resource.Get(), stateBefore, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		}

		trackedState.CurrentState = newState;

		return {};
	}

	Status ResourceStateTracker::setPermanentTextureState(
		Texture& texture,
		ResourceStateFlags state)
	{
		if (texture.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return validateTextureState(texture, state);
		}

		SPALL_TRY(requireTextureState(texture, state));

		trackedTextureState(texture).PermanentTransition = true;
		m_PermanentTextureStates.emplace_back(&texture, state);

		return {};
	}

	Status ResourceStateTracker::setPermanentBufferState(
		Buffer& buffer,
		ResourceStateFlags state)
	{
		if (buffer.m_PermanentState != ResourceStateFlags::Unknown)
		{
			return validateBufferState(buffer, state);
		}

		SPALL_TRY(requireBufferState(buffer, state));

		trackedBufferState(buffer).PermanentTransition = true;
		m_PermanentBufferStates.emplace_back(&buffer, state);

		return {};
	}

	Status ResourceStateTracker::commitBarriers()
	{
		if (m_PendingBarriers.empty())
		{
			return {};
		}

		if (m_CommandList == nullptr)
		{
			return ERR_INVALID_STATE;
		}

		m_CommandList->ResourceBarrier(static_cast<UINT>(m_PendingBarriers.size()), m_PendingBarriers.data());
		m_PendingBarriers.clear();

		return {};
	}

	bool ResourceStateTracker::hasPendingBarriers() const
	{
		return not m_PendingBarriers.empty();
	}

	void ResourceStateTracker::commandListSubmitted()
	{
		for (const auto& trackedState : m_TextureStates)
		{
			trackedState.first->m_SubresourceStates = trackedState.second.Subresources;
		}

		for (const auto& trackedState : m_BufferStates)
		{
			trackedState.first->m_State = trackedState.second.CurrentState;
		}

		for (const auto& permanentState : m_PermanentTextureStates)
		{
			permanentState.first->m_PermanentState = permanentState.second;
			permanentState.first->m_SubresourceStates.assign(
				permanentState.first->m_SubresourceStates.size(),
				permanentState.second);
		}

		for (const auto& permanentState : m_PermanentBufferStates)
		{
			permanentState.first->m_PermanentState = permanentState.second;
			permanentState.first->m_State = permanentState.second;
		}

		m_PermanentTextureStates.clear();
		m_PermanentBufferStates.clear();
	}

	void ResourceStateTracker::reset()
	{
		m_TextureStates.clear();
		m_BufferStates.clear();
		m_PendingBarriers.clear();
		m_PermanentTextureStates.clear();
		m_PermanentBufferStates.clear();
	}
} // namespace spall::d3d12
