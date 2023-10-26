#include "Platform/Window.hpp"
#include "Debug/Logger.hpp"
#include "ProjectInfo.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef CreateWindow

namespace gsim {
	// Constants
	static const char* const CLASS_NAME = "gsimWindow";

	// Variables
	static WindowInfo windowInfo;
	static WindowPlatformInfo platformInfo;
	static bool windowDestroyed = false;

	static Event moveEvent;
	static Event resizeEvent;

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
		winClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
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

		GSIM_LOG_INFO("Creates Win32 window");
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

			break;
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

			break;
		}
		case WM_CLOSE:
			// Destroy the window
			::DestroyWindow(platformInfo.hWindow);

			break;
		case WM_DESTROY:
			// Remember that the window was destroyed
			windowDestroyed = true;

			break;
		}

		return DefWindowProcA(hWnd, message, wParam, lParam);
	}

	// Public functions
	void CreateWindow() {
		RegisterWindowClass();
		CreateWin32Window();
	}
	void DestroyWindow() {
		// No action required to destroy the window
	}
	bool PollWindowEvents() {
		// Loop through every pending message
		MSG message;
		while(PeekMessageA(&message, platformInfo.hWindow, 0, 0, PM_REMOVE)) {
			// Dispatch the message
			DispatchMessageA(&message);
		}

		return !windowDestroyed;
	}

	Event& GetWindowMoveEvent() {
		return moveEvent;
	}
	Event& GetWindowResizeEvent() {
		return resizeEvent;
	}

	WindowInfo GetWindowInfo() {
		return windowInfo;
	}
	WindowPlatformInfo GetWindowPlatformInfo() {
		return platformInfo;
	}
}