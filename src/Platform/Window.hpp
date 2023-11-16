#pragma once

#include <stdint.h>
#include "Utils/BuildInfo.hpp"
#include "Utils/Event.hpp"

#if defined(GSIM_PLATFORM_WINDOWS)
#include <windef.h>
#endif

namespace gsim {
	/// @brief A struct containing general information about the window.
	struct WindowInfo {
		/// @brief The window's X position.
		int32_t posX;
		/// @brief The window's Y position.
		int32_t posY;
		/// @brief The window's width.
		uint32_t width;
		/// @brief The window's height.
		uint32_t height;
		/// @brief True if the window is maximized, otherwise false.
		bool isMaximized;
		/// @brief True if the window is minimized, otherwise false.
		bool isMinimized;
	};

#if defined(GSIM_PLATFORM_WINDOWS)
	/// @brief A struct containing Windows-specific window info.
	struct WindowPlatformInfo {
		/// @brief A handle to the app's instance.
		HINSTANCE hInstance;
		/// @brief The unique identifier of the window's class.
		ATOM winClassAtom;
		/// @brief A handle to the app's window.
		HWND hWindow;
	};
#endif

	/// @brief A struct containing the info given on window move events.
	struct WindowMoveEventInfo {
		/// @brief The X component of the window's new position.
		int32_t newX;
		/// @brief The Y component of the window's new position.
		int32_t newY;
	};
	/// @brief A struct containing the info given on window resize events.
	struct WindowResizeEventInfo {
		/// @brief The new width of the window.
		uint32_t newWidth;
		/// @brief The new height of the window.
		uint32_t newHeight;
		/// @brief True if the window was maximized, otherwise false.
		bool windowMaximized;
		/// @brief True if the window was unmaximized, otherwise false.
		bool windowUnmaximized;
		/// @brief True if the window was minimized, otherwise false.
		bool windowMinimized;
		/// @brief True if the window was unminimized, otherwise false.
		bool windowUnminimized;
	};

	/// @brief Creates the program's window.
	void CreateWindow();
	/// @brief Destroys the program's window.
	void DestroyWindow();
	/// @brief Polls the window's events
	/// @return False if the window was closed, otherwise true.
	void PollWindowEvents();

	/// @brief Gets the window's move event.
	/// @return A reference to the window's move event.
	Event& GetWindowMoveEvent();
	/// @brief Gets the window's resize event.
	/// @return A reference to the window's resize event.
	Event& GetWindowResizeEvent();
	/// @brief Gets the window's draw event.
	/// @return A reference to the window's draw event.
	Event& GetWindowDrawEvent();

	/// @brief Gets the window's general info.
	/// @return A struct containing the window's general info.
	WindowInfo GetWindowInfo();
	/// @brief Gets the window's platform-specific info.
	/// @return A struct containing the window's platform-specific info.
	WindowPlatformInfo GetWindowPlatformInfo();
}