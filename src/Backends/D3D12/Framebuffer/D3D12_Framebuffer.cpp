#include <src/Backends/D3D12/Framebuffer/D3D12_Framebuffer.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>

namespace spall::d3d12
{
	Framebuffer::Framebuffer(
		Device& device,
		const FramebufferInfo& info,
		TextureView* colorViews[MaxColorAttachments],
		std::uint32_t colorCount,
		TextureView* depthView,
		TextureView* resolveViews[MaxColorAttachments])
		: m_Device(&device), m_Info(info), m_ColorCount(colorCount), m_DepthView(depthView)
	{
		for (std::uint32_t attachmentIndex = 0; attachmentIndex < colorCount; ++attachmentIndex)
		{
			m_ColorViews[attachmentIndex].reset(colorViews[attachmentIndex]);
			m_ResolveViews[attachmentIndex].reset(resolveViews[attachmentIndex]);
		}
	}

	Framebuffer::~Framebuffer() = default;

	RenderBackendType Framebuffer::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	FramebufferInfo Framebuffer::info() const
	{
		return m_Info;
	}
} // namespace spall::d3d12
