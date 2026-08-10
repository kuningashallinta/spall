#pragma once

#include <windows.h>

/// A swap chain needs a window; this one is never shown.
class HiddenWindow
{
public:
	HiddenWindow(
		void)
	{
		WNDCLASSEXW windowClass = {};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = DefWindowProcW;
		windowClass.hInstance = GetModuleHandleW(nullptr);
		windowClass.lpszClassName = L"SpallRHIHiddenTestWindow";
		RegisterClassExW(&windowClass);

		m_Window = CreateWindowExW(
			0,
			windowClass.lpszClassName,
			L"SpallRHIHiddenTestWindow",
			WS_OVERLAPPEDWINDOW,
			0,
			0,
			256,
			256,
			nullptr,
			nullptr,
			windowClass.hInstance,
			nullptr);
	}

	~HiddenWindow(
		void)
	{
		if (m_Window != nullptr)
		{
			DestroyWindow(m_Window);
		}
	}

	HiddenWindow(const HiddenWindow&) = delete;
	HiddenWindow& operator=(const HiddenWindow&) = delete;

	HWND handle(
		void) const
	{
		return m_Window;
	}

private:
	HWND m_Window = nullptr;
};
