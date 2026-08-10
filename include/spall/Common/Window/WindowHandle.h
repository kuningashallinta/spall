// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

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
