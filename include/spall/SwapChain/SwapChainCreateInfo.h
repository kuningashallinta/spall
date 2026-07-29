#pragma once

#include <spall/Common/Enums/AlphaMode.h>
#include <spall/Common/Enums/PresentMode.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Window/WindowHandle.h>

#include <cstdint>

namespace spall
{
	/// Describes a swap chain attached to a native window.
	struct SwapChainCreateInfo
	{
		/// Native window handle used only during creation.
		WindowHandle Window = {};
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
		spall::Format Format = spall::Format::Unknown;
		PresentMode PresentMode = PresentMode::VSync;
		AlphaMode AlphaMode = AlphaMode::Opaque;
	};
} // namespace spall
