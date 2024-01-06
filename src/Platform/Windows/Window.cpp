#include "Utils/BuildInfo.hpp"

#ifdef GSIM_PLATFORM_WINDOWS

#include "Platform/Window.hpp"
#include "Debug/Logger.hpp"
#include "Manager/Parser.hpp"
#include "ProjectInfo.hpp"
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef CreateWindow

namespace gsim {
	// Constants
	static const char* const CLASS_NAME = "gsimWindow";

	// Variables
	static WindowInfo windowInfo;
	static WindowPlatformInfo platformInfo;

	static Event moveEvent;
	static Event resizeEvent;
	static Event drawEvent;

	// WinProc declaration
	static LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam);

	// Internal helper functions
	static void RegisterWindowClass() {
		// Get the handle to the current instance
		platformInfo.hInstance = GetModuleHandle(nullptr);

		// Set the class info
		WNDCLASSEXA winClass;

		winClass.cbSize = sizeof(WNDCLASSEXA);
		winClass.style = CS_HREDRAW | CS_VREDRAW;
		winClass.lpfnWndProc = WinProc;
		winClass.cbClsExtra = 0;
		winClass.cbWndExtra = 0;
		winClass.hInstance = platformInfo.hInstance;
		winClass.hIcon = LoadIcon(platformInfo.hInstance, IDI_APPLICATION);
		winClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		winClass.hbrBackground = nullptr;
		winClass.lpszMenuName = nullptr;
		winClass.lpszClassName = CLASS_NAME;
		winClass.hIconSm = LoadIcon(platformInfo.hInstance, IDI_APPLICATION);

		// Register the class
		platformInfo.winClassAtom = RegisterClassExA(&winClass);

		if(!platformInfo.winClassAtom)
			GSIM_LOG_FATAL("Failed to register Win32 window class!");
		
		GSIM_LOG_INFO("Registered Win32 window class.");
	}
	static void CreateWin32Window() {
		// Create the window
		platformInfo.hWindow = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, (LPCSTR)(size_t)platformInfo.winClassAtom, GSIM_PROJECT_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, platformInfo.hInstance, nullptr);

		if(!platformInfo.hWindow)
			GSIM_LOG_FATAL("Failed to create Win32 window!");
		
		// Show the window
		ShowWindow(platformInfo.hWindow, SW_SHOWNORMAL);

		// Get the window's position and size
		POINT windowPoint{ 0, 0 };
		RECT clientRect;
		ClientToScreen(platformInfo.hWindow, &windowPoint);
		GetClientRect(platformInfo.hWindow, &clientRect);

		windowInfo.posX = (int32_t)windowPoint.x;
		windowInfo.posY = (int32_t)windowPoint.y;
		windowInfo.width = (uint32_t)clientRect.right;
		windowInfo.height = (uint32_t)clientRect.bottom;
		windowInfo.isMaximized = false;
		windowInfo.isMinimized = false;

		GSIM_LOG_INFO("Created Win32 window");
	}

	static LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {
		switch(message) {
		case WM_SIZE: {
			// Set the window's new size
			windowInfo.width = (uint32_t)LOWORD(lParam);
			windowInfo.height = (uint32_t)HIWORD(lParam);

			// Set the window resize event info
			WindowResizeEventInfo resizeInfo;

			resizeInfo.newWidth = windowInfo.width;
			resizeInfo.newHeight = windowInfo.height;

			switch(wParam) {
			case SIZE_RESTORED:
				// Set the resize info flags
				resizeInfo.windowMaximized = false;
				resizeInfo.windowUnmaximized = windowInfo.isMaximized;
				resizeInfo.windowMinimized = false;
				resizeInfo.windowUnminimized = windowInfo.isMinimized;
				
				// Set the maximized and minimized flags
				windowInfo.isMaximized = false;
				windowInfo.isMinimized = false;
				
				break;
			case SIZE_MINIMIZED:
				// Set the resize info flags
				resizeInfo.windowMaximized = false;
				resizeInfo.windowUnmaximized = false;
				resizeInfo.windowMinimized = true;
				resizeInfo.windowUnminimized = false;
				
				// Set the minimized flag
				windowInfo.isMinimized = true;
				
				break;
			case SIZE_MAXIMIZED:
				// Set the resize info flags
				resizeInfo.windowMaximized = true;
				resizeInfo.windowUnmaximized = false;
				resizeInfo.windowMinimized = false;
				resizeInfo.windowUnminimized = false;
				
				// Set the maximized flag
				windowInfo.isMaximized = true;
				
				break;
			}

			// Call the window resize event
			resizeEvent.CallEvent(&resizeInfo);

			return 0;
		}
		case WM_MOVE: {
			// Set the window's new position
			windowInfo.posX = (int32_t)(int16_t)LOWORD(lParam);
			windowInfo.posY = (int32_t)(int16_t)HIWORD(lParam);

			// Set the window move event info
			WindowMoveEventInfo moveInfo;

			moveInfo.newX = windowInfo.posX;
			moveInfo.newY = windowInfo.posY;

			// Call the window move event
			moveEvent.CallEvent(&moveEvent);

			return 0;
		}
		case WM_PAINT:
			// Call the window draw event
			drawEvent.CallEvent(nullptr);

			return 0;
		case WM_CLOSE:
			// Destroy the window
			::DestroyWindow(platformInfo.hWindow);

			return 0;
		case WM_DESTROY:
			// Post the quit message
			PostQuitMessage(0);

			return 0;
		}

		return DefWindowProcA(hWnd, message, wParam, lParam);
	}

	// Public functions
	void CreateWindow() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		RegisterWindowClass();
		CreateWin32Window();
	}
	void DestroyWindow() {
		// No action required to destroy the window
	}
	void PollWindowEvents() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Loop through every pending message
		MSG message;
		while(GetMessageA(&message, nullptr, 0, 0)) {
			// Dispatch the message
			DispatchMessageA(&message);
		}
	}

	Event& GetWindowMoveEvent() {
		return moveEvent;
	}
	Event& GetWindowResizeEvent() {
		return resizeEvent;
	}
	Event& GetWindowDrawEvent() {
		return drawEvent;
	}

	WindowInfo GetWindowInfo() {
		return windowInfo;
	}
	WindowPlatformInfo GetWindowPlatformInfo() {
		return platformInfo;
	}

	void CloseWindow() {
		// Destroy the window
		::DestroyWindow(platformInfo.hWindow);
	}
}

#endif