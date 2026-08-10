// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>

namespace spall::vk
{
	class Device;
	class TextureView;
	class CommandList;

	class Framebuffer final : public SharedObject<IFramebuffer>
	{
	public:
		Framebuffer(
			Device& device,
			const FramebufferInfo& info,
			TextureView* colorViews[MaxColorAttachments],
			std::uint32_t colorCount,
			TextureView* depthView,
			TextureView* resolveViews[MaxColorAttachments]);

		~Framebuffer(void) override;

		RenderBackendType backendType(void) const override;
		FramebufferInfo info(void) const override;

	private:
		Resource<Device> m_Device;

		FramebufferInfo m_Info = {};

		Resource<TextureView> m_ColorViews[MaxColorAttachments];
		std::uint32_t m_ColorCount = 0;
		Resource<TextureView> m_DepthView;
		Resource<TextureView> m_ResolveViews[MaxColorAttachments];

		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

	private:
		friend class Device;
		friend class CommandList;
	};
} // namespace spall::vk
