#pragma once

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Common/Status/Status.h>
#include <spall/Resources/Texture/TextureInfo.h>
#include <spall/Resources/Texture/TextureSubresourceRange.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>
#include <src/Validation/Common/TextureValidation.h>

#include <unordered_map>
#include <utility>
#include <vector>

namespace spall::d3d12
{
	class Buffer;
	class Texture;

	/// Tracks per-subresource texture states and whole-buffer states.
	class ResourceStateTracker
	{
	public:
		void setCommandList(ID3D12GraphicsCommandList* commandList);

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

		Status commitBarriers(void);
		bool hasPendingBarriers(void) const;

		void commandListSubmitted(void);
		void reset(void);

	private:
		struct ResourceState
		{
			ResourceStateFlags CurrentState = ResourceStateFlags::Unknown;
			bool PermanentTransition = false;
		};

		struct TextureState
		{
			std::vector<ResourceStateFlags> Subresources;
			bool PermanentTransition = false;
		};

		static ResourceStateFlags globalSubresourceState(
			const Texture& texture,
			std::uint32_t subresource);

		static ResourceStateFlags globalBufferState(const Buffer& buffer);

		TextureState& trackedTextureState(Texture& texture);
		ResourceState& trackedBufferState(Buffer& buffer);

		void appendTransition(
			ID3D12Resource& resource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			UINT subresource);

		ID3D12GraphicsCommandList* m_CommandList = nullptr;

		std::unordered_map<Texture*, TextureState> m_TextureStates;
		std::unordered_map<Buffer*, ResourceState> m_BufferStates;

		std::vector<D3D12_RESOURCE_BARRIER> m_PendingBarriers;

		std::vector<std::pair<Texture*, ResourceStateFlags>> m_PermanentTextureStates;
		std::vector<std::pair<Buffer*, ResourceStateFlags>> m_PermanentBufferStates;
	};
} // namespace spall::d3d12
