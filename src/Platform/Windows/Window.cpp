#include <stddef.h>

#ifdef WIN32

#include "Platform/Window.hpp"
#include "Debug/Exception.hpp"
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// HWND hash
template<>
struct std::hash<HWND> {
	std::size_t operator()(HWND hWnd) const noexcept {
		return std::hash<size_t>()((size_t)hWnd);
	}
};

namespace gsim {
	// Constants
	static const size_t MAX_ERROR_MESSAGE_SIZE = 256;

	// Internal variables
	static std::unordered_map<HWND, Window*> windowMap;

	// Internal functions
	LRESULT Window::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// Get the window pointer from the HWND
		Window* window;

		std::unordered_map<HWND, Window*>::iterator windowIter = windowMap.find(hWnd);
		if(windowIter != windowMap.end()) {
			window = windowIter->second;
		} else {
			window = nullptr;
		}

		// Process the message, if the window is known
		if(window) {
			switch(msg) {
			case WM_CHAR: {
				// Set the key event info
				KeyEventInfo eventInfo {
					.key = (char)wParam,
					.repeatCount = (uint32_t)LOWORD(lParam)
				};

				// Call the key event
				window->keyEvent.CallEvent(&eventInfo);

				break;
			}
			case WM_SIZE:
				// Set the window's new size
				window->windowInfo.width = (uint32_t)LOWORD(lParam);
				window->windowInfo.height = (uint32_t)HIWORD(lParam);

				// Call the window resize event
				window->resizeEvent.CallEvent(nullptr);

				break;
			case WM_ENTERSIZEMOVE:
			 	// Set the resizing variable to true
				window->windowInfo.resizing = true;

				break;
			case WM_EXITSIZEMOVE:
				// Set the resizing variable to false
				window->windowInfo.resizing = false;

				break;
			case WM_PAINT:
				// Call the window draw event
				window->drawEvent.CallEvent(nullptr);

				return 0;
			case WM_CLOSE:
				// Set the window as not running
				window->windowInfo.running = false;
				return 0;
			}
		}

		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	// Public functions
	Window::Window(const char* name, uint32_t width, uint32_t height) : windowInfo({ width, height, true }) {
		// Get the program's HINSTANCE
		platformInfo.hInstance = GetModuleHandleA(nullptr);

		// CCreate the background brush
		platformInfo.bgBrush = CreateSolidBrush(RGB(0, 0, 0));

		// Set the window's class info
		WNDCLASSEXA winClassInfo;

		winClassInfo.cbSize = sizeof(WNDCLASSEXA);
		winClassInfo.style = 0;
		winClassInfo.lpfnWndProc = WindowProc;
		winClassInfo.cbClsExtra = 0;
		winClassInfo.cbWndExtra = 0;
		winClassInfo.hInstance = platformInfo.hInstance;
		winClassInfo.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
		winClassInfo.hCursor = LoadCursorA(nullptr, IDC_ARROW);
		winClassInfo.hbrBackground = platformInfo.bgBrush;
		winClassInfo.lpszMenuName = nullptr;
		winClassInfo.lpszClassName = name;
		winClassInfo.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);

		// Register the window's class
		platformInfo.winClassID = RegisterClassExA(&winClassInfo);

		if(!platformInfo.winClassID) {
			// Format the error message and throw an exception
			char err[MAX_ERROR_MESSAGE_SIZE] = "Unknown.";
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), LANG_SYSTEM_DEFAULT, err, MAX_ERROR_MESSAGE_SIZE, nullptr);

			GSIM_THROW_EXCEPTION("Failed to register Win32 window class! Error: %s", err);
		}

		// Create the window
		platformInfo.hWnd = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, (LPCSTR)(size_t)platformInfo.winClassID, name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowInfo.width, windowInfo.height, nullptr, nullptr, platformInfo.hInstance, nullptr);

		if(!platformInfo.hWnd) {
			// Format the error message and throw an exception
			char err[MAX_ERROR_MESSAGE_SIZE] = "Unknown.";
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), LANG_SYSTEM_DEFAULT, err, MAX_ERROR_MESSAGE_SIZE, nullptr);

			GSIM_THROW_EXCEPTION("Failed to create Win32 window! Error: %s", err);
		}

		// Add the window to the map
		windowMap.insert(std::make_pair(platformInfo.hWnd, this));

		// Show the window
		ShowWindow(platformInfo.hWnd, SW_SHOWNORMAL);
	}

	bool Window::IsMouseDown() const {
		return !windowInfo.resizing && (GetActiveWindow() == platformInfo.hWnd) && (GetKeyState(VK_LBUTTON) >> 15) & 1;
	}
	Window::MousePos Window::GetMousePos() const {
		// Get the mouse's position relative to the window
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(platformInfo.hWnd, &point);

		return { (int32_t)point.x, (int32_t)point.y };
	}

	void Window::ParseEvents() {
		// Handle the window's events
		MSG msg;
		while(windowInfo.running && PeekMessageA(&msg, platformInfo.hWnd, 0, 0, PM_REMOVE)) {
			// Translate and dispatch the message
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}
	void Window::CloseWindow() {
		// Post a close message
		PostMessageA(platformInfo.hWnd, WM_CLOSE, 0, 0);
	}

	Window::~Window() {
		// Remove the window from the map
		windowMap.erase(platformInfo.hWnd);

		// Destroy the all of the window's components
		DestroyWindow(platformInfo.hWnd);
		UnregisterClassA((LPCSTR)(size_t)platformInfo.winClassID, platformInfo.hInstance);
		DeleteObject(platformInfo.bgBrush);
	}
}

#endif