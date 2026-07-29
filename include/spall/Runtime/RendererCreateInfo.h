#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Device/DeviceCreateInfo.h>
#include <spall/SwapChain/SwapChainCreateInfo.h>

namespace spall
{
	/// Describes a managed renderer attached to a caller-owned window.
	struct RendererCreateInfo
	{
		RenderBackendType Backend = RenderBackendType::D3D12;
		DeviceCreateInfo Device = {};
		SwapChainCreateInfo SwapChain = {};

		/// Optional depth-stencil format for the default frame pass.
		Format DepthStencilFormat = Format::Unknown;
	};
} // namespace spall
