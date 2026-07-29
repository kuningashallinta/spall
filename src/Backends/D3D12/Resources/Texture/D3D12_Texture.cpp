#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Validation/Common/TextureValidation.h>

#include <utility>

namespace spall::d3d12
{
	Texture::Texture(
		Device& device,
		const TextureInfo& info,
		ComPtr<ID3D12Resource> resource,
		SwapChain* swapChain)
		: m_Device(&device), m_SwapChain(swapChain), m_Info(info), m_Resource(std::move(resource)),
		  m_IsSwapChainTexture(swapChain != nullptr)
	{
		m_SubresourceStates.assign(textureSubresourceCount(info), info.InitialState);

		if (info.DebugName != nullptr)
		{
			m_DebugName = info.DebugName;
			m_Info.DebugName = m_DebugName.c_str();
		}
		else
		{
			m_Info.DebugName = nullptr;
		}
	}

	Texture::~Texture() = default;

	RenderBackendType Texture::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	TextureInfo Texture::info() const
	{
		return m_Info;
	}
} // namespace spall::d3d12
