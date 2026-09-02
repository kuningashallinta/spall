// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Resources/Texture/ITexture1D.h>
#include <spall/Resources/Texture/ITexture2D.h>
#include <spall/Resources/Texture/ITexture3D.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
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

	/// Resource storage shared by every texture dimension.
	///
	/// This is not an ITexture. Texture1D, Texture2D and Texture3D each implement
	/// one public interface and inherit this storage, so backend code holding an
	/// ITexture reaches the resource through textureStorage().
	class Texture
	{
	public:
		Texture(
			Device& device,
			const TextureInfo& info,
			ComPtr<ID3D12Resource> resource,
			SwapChain* swapChain = nullptr);

		~Texture(void);

	protected:
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

		friend ITexture* textureInterface(Texture& storage);
	};

	class Texture1D final : public SharedObject<ITexture1D>, public Texture
	{
	public:
		Texture1D(
			Device& device,
			const TextureInfo& info,
			ComPtr<ID3D12Resource> resource);

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
			ComPtr<ID3D12Resource> resource,
			SwapChain* swapChain = nullptr);

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
			ComPtr<ID3D12Resource> resource);

		RenderBackendType backendType(void) const override;
		TextureType type(void) const override;
		TextureInfo info(void) const override;

		std::uint32_t width(void) const override;
		std::uint32_t height(void) const override;
		std::uint32_t depth(void) const override;
	};

	/// Gets the resource storage behind a texture of any dimension.
	/// Returns null when the texture belongs to a different backend.
	Texture* textureStorage(ITexture& texture);

	Texture* textureStorage(ITexture* texture);

	/// Gets the interface object that owns a texture's resource storage.
	ITexture* textureInterface(Texture& storage);
} // namespace spall::d3d12
