#pragma once

#include <stdint.h>
#include "Utils/BuildInfo.hpp"
#include "Utils/Event.hpp"

#if defined(GSIM_PLATFORM_WINDOWS)
#include <windef.h>
#endif

namespace gsim {
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
	};

	/// @brief Creates the program's window.
	void CreateWindow();
	/// @brief Destroys the program's window.
	void DestroyWindow();
	/// @brief Polls the window's events
	/// @return False if the window was closed, otherwise true.
	bool PollWindowEvents();

	/// @brief Gets the window's move event.
	/// @return A reference to the window's move event.
	Event& GetWindowMoveEvent();
	/// @brief Gets the window's resize event.
	/// @return A reference to the window's resize event.
	Event& GetWindowResizeEvent();

	/// @brief Returns the window's platform-specific info.
	/// @return A struct containing the window's platform-specific info.
	WindowPlatformInfo GetWindowPlatformInfo();
}