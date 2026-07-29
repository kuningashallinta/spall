#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Resources/Texture/ITexture.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <string>
#include <vector>

namespace spall::d3d12
{
	class CommandList;
	class Device;
	class Frame;
	class GraphicsQueue;
	class ResourceStateTracker;
	class SwapChain;
	class TextureView;

	class Texture final : public SharedObject<ITexture>
	{
	public:
		Texture(
			Device& device,
			const TextureInfo& info,
			ComPtr<ID3D12Resource> resource,
			SwapChain* swapChain = nullptr);

		~Texture(void) override;

		RenderBackendType backendType(void) const override;
		TextureInfo info(void) const override;

	private:
		Resource<Device> m_Device;
		SwapChain* m_SwapChain = nullptr;

		std::string m_DebugName;
		TextureInfo m_Info = {};

		ComPtr<ID3D12Resource> m_Resource;

		/// Per-subresource state left by the most recently submitted command list.
		std::vector<ResourceStateFlags> m_SubresourceStates;

		ResourceStateFlags m_PermanentState = ResourceStateFlags::Unknown;
		bool m_IsSwapChainTexture = false;

	private:
		friend class CommandList;
		friend class Device;
		friend class Frame;
		friend class GraphicsQueue;
		friend class ResourceSet;
		friend class ResourceStateTracker;
		friend class SwapChain;
		friend class TextureView;
	};
} // namespace spall::d3d12
