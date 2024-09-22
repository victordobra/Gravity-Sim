#pragma once

#include "Event.hpp"
#include <stddef.h>
#include <stdint.h>

#if defined(WIN32)
#include <windef.h>
#endif

namespace gsim {
	/// @brief An implementation of a normal window to be used by the program.
	class Window {
	public:
		/// @brief A struct containing the window's general info.
		struct WindowInfo {
			/// @brief The window's width.
			uint32_t width;
			/// @brief The window's height.
			uint32_t height;
			/// @brief True if the window is running, otherwise false.
			bool running;
			/// @brief True if the window is being resized, otherwise false.
			bool resizing;
		};

#if defined(WIN32)
		/// @brief A struct containing the window's Windows specific info.
		struct PlatformInfo {
			/// @brief The handle to the Windows instance.
			HINSTANCE hInstance;
			/// @brief The handle to the brush used for the window's background.
			HBRUSH bgBrush;
			/// @brief The window's class ID.
			ATOM winClassID;
			/// @brief The handle to the window.
			HWND hWnd;
		};
#endif
		/// @brief A struct containing the info passed on a key event.
		struct KeyEventInfo {
			/// @brief The character represented by the pressed key.
			char key;
			/// @brief The repeat count for the key.
			uint32_t repeatCount;
		};
		/// @brief A struct containing the mouse's position.
		struct MousePos {
			/// @brief The X coordinate of the mouse's position.
			int32_t x;
			/// @brief The X coordinate of the mouse's position.
			int32_t y;
		};

		Window() = delete;
		Window(const Window&) = delete;
		Window(Window&&) noexcept = delete;
		/// @brief Creates a window.
		/// @param name The window's name.
		/// @param width The window's width.
		/// @param height The window's height.
		Window(const char* name, uint32_t width, uint32_t height);

		Window& operator=(const Window&) = delete;
		Window& operator=(Window&&) = delete;

		/// @brief Gets the window's info.
		/// @return A struct containing the window's info.
		const WindowInfo& GetWindowInfo() const {
			return windowInfo;
		}
		/// @brief Gets the window's platform specific info.
		/// @return A struct containing the window's platform specific info.
		const PlatformInfo& GetPlatformInfo() const {
			return platformInfo;
		}

		/// @brief Gets the window's resize event.
		/// @return A reference to the window's resize event.
		Event& GetResizeEvent() {
			return resizeEvent;
		}
		/// @brief Gets the window's resize event.
		/// @return A const reference to the window's resize event.
		const Event& GetResizeEvent() const {
			return resizeEvent;
		}
		/// @brief Gets the window's draw event.
		/// @return A reference to the window's draw event.
		Event& GetDrawEvent() {
			return drawEvent;
		}
		/// @brief Gets the window's draw event.
		/// @return A const reference to the window's draw event.
		const Event& GetDrawEvent() const {
			return drawEvent;
		}
		/// @brief Gets the window's key event.
		/// @return A reference to the window's key event.
		Event& GetKeyEvent() {
			return keyEvent;
		}
		/// @brief Gets the window's key event.
		/// @return A const reference to the window's key event.
		const Event& GetKeyEvent() const {
			return keyEvent;
		}

		/// @brief Gets the mouse's position.
		/// @return The mouse's position.
		MousePos GetMousePos() const;
		/// @brief Checks if the mouse is currently pressed.
		/// @return True if the mouse is currently pressed, otherwise false.
		bool IsMouseDown() const;

		/// @brief Parses all the window's queued events.
		void ParseEvents();
		/// @brief Closes the window.
		void CloseWindow();

		/// @brief Destroys the window.
		~Window();
	private:
#ifdef WIN32
		static LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

		WindowInfo windowInfo;
		PlatformInfo platformInfo;

		Event resizeEvent;
		Event drawEvent;
		Event keyEvent;
	};
}