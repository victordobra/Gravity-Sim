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
	static HINSTANCE hInstance;
	static ATOM winClassAtom;
	static HWND hWindow;
	static bool windowDestroyed = false;

	static Event moveEvent;
	static Event resizeEvent;

	// WinProc declaration
	static LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam);

	// Internal helper functions
	static void RegisterWindowClass() {
		// Get the handle to the current instance
		hInstance = GetModuleHandle(nullptr);

		// Set the class info
		WNDCLASSEXA winClass;

		winClass.cbSize = sizeof(WNDCLASSEXA);
		winClass.style = CS_HREDRAW | CS_VREDRAW;
		winClass.lpfnWndProc = WinProc;
		winClass.cbClsExtra = 0;
		winClass.cbWndExtra = 0;
		winClass.hInstance = hInstance;
		winClass.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
		winClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		winClass.hbrBackground = nullptr;
		winClass.lpszMenuName = nullptr;
		winClass.lpszClassName = CLASS_NAME;
		winClass.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

		// Register the class
		winClassAtom = RegisterClassExA(&winClass);

		if(!winClassAtom)
			GSIM_LOG_FATAL("Failed to register Win32 window class!");
		
		GSIM_LOG_INFO("Registered Win32 window class.");
	}
	static void CreateWin32Window() {
		// Create the window
		hWindow = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, (LPCSTR)(size_t)winClassAtom, GSIM_PROJECT_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);

		if(!hWindow)
			GSIM_LOG_FATAL("Failed to create Win32 window!");
		
		// Show the window
		ShowWindow(hWindow, SW_SHOWDEFAULT);

		GSIM_LOG_INFO("Creates Win32 window");
	}

	static LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {
		switch(message) {
		case WM_SIZE: {
			// Set the window resize event info
			WindowResizeEventInfo resizeInfo;

			resizeInfo.newWidth = (uint32_t)LOWORD(lParam);
			resizeInfo.newHeight = (uint32_t)HIWORD(lParam);

			// Call the window resize event
			resizeEvent.CallEvent(&resizeInfo);

			break;
		}
		case WM_MOVE: {
			// Set the window move event info
			WindowMoveEventInfo moveInfo;

			moveInfo.newX = (int32_t)(int16_t)LOWORD(lParam);
			moveInfo.newY = (int32_t)(int16_t)HIWORD(lParam);

			// Call the window move event
			moveEvent.CallEvent(&moveEvent);

			break;
		}
		case WM_CLOSE:
			// Destroy the window
			::DestroyWindow(hWindow);

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
		while(PeekMessageA(&message, hWindow, 0, 0, PM_REMOVE)) {
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

	WindowPlatformInfo GetWindowPlatformInfo() {
		return { hInstance, winClassAtom, hWindow };
	}
}