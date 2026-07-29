#pragma once

#include <spall/Common/Enums/WindowHandleType.h>

namespace spall
{
	struct WindowHandle
	{
		WindowHandleType Type = WindowHandleType::Win32;
		void* Value = nullptr;
	};
} // namespace spall
