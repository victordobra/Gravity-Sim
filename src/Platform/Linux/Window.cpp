#include "Utils/BuildInfo.hpp"

#ifdef GSIM_PLATFORM_LINUX

#include "Platform/Window.hpp"
#include "Debug/Logger.hpp"
#include "Manager/Parser.hpp"
#include "ProjectInfo.hpp"
#include <stdint.h>
#include <string.h>

#include <X11/Xlib.h>

namespace gsim {
	// Constants
	const int32_t DEFAULT_X = 100;
	const int32_t DEFAULT_Y = 100;
	const uint32_t DEFAULT_WIDTH = 1000;
	const uint32_t DEFAULT_HEIGHT = 1000;

	const uint32_t EVENT_MASK = XCB_CW_EVENT_MASK;
	const uint32_t EVENT_VALUES = XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	const char WM_PROTOCOLS_ATOM_NAME[] = "WM_PROTOCOLS";
	const char WM_DELETE_MSG_ATOM_NAME[] = "WM_DELETE_WINDOW";
	const char WM_STATE_ATOM_NAME[] = "_NET_WM_STATE";
	const char WM_MINIMIZED_ATOM_NAME[] = "_NET_WM_STATE_HIDDEN";
	const char WM_MAXIMIZED_H_ATOM_NAME[] = "_NET_WM_STATE_MAXIMIZED_HORIZ";
	const char WM_MAXIMIZED_V_ATOM_NAME[] = "_NET_WM_STATE_MAXIMIZED_VERT";

	// Internal variables
	static WindowInfo windowInfo;
	static WindowPlatformInfo platformInfo;

	static xcb_atom_t wmProtocolsAtom;
	static xcb_atom_t wmDeleteMsgAtom;
	static xcb_atom_t wmStateAtom;
	static xcb_atom_t wmMinimizedAtom;
	static xcb_atom_t wmMaximizedHAtom;
	static xcb_atom_t wmMaximizedVAtom;

	static Event moveEvent;
	static Event resizeEvent;
	static Event drawEvent;

	static bool running = true;

	// Internal helper functions
	static void ConnectToX() {
		// Create the X connection and get the screen index
		int32_t screenIndex;

		platformInfo.connection = xcb_connect(nullptr, &screenIndex);
		if(xcb_connection_has_error(platformInfo.connection))
			GSIM_LOG_FATAL("Failed to connect to X server!");

		// Get data from the X server
		const xcb_setup_t* setup = xcb_get_setup(platformInfo.connection);

		// Loop through the screens until the wanted index is encountered
		xcb_screen_iterator_t screenIter = xcb_setup_roots_iterator(setup);
		for(int32_t i = 0; i != screenIndex; ++i)
			xcb_screen_next(&screenIter);
		
		platformInfo.screen = screenIter.data;

		GSIM_LOG_INFO("Connected to X server.");
	}
	static void CreateXWindow() {
		// Create the window
		platformInfo.window = xcb_generate_id(platformInfo.connection);
		xcb_create_window(platformInfo.connection, (uint8_t)XCB_COPY_FROM_PARENT, platformInfo.window, platformInfo.screen->root, (int16_t)DEFAULT_X, (int16_t)DEFAULT_Y, (uint16_t)DEFAULT_WIDTH, (uint16_t)DEFAULT_HEIGHT, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, platformInfo.screen->root_visual, EVENT_MASK, &EVENT_VALUES);

		// Get the window atoms
		xcb_intern_atom_cookie_t wmProtocolsCookie = xcb_intern_atom(platformInfo.connection, false, sizeof(WM_PROTOCOLS_ATOM_NAME) - 1, WM_PROTOCOLS_ATOM_NAME);
		xcb_intern_atom_cookie_t wmDeleteMsgCookie = xcb_intern_atom(platformInfo.connection, false, sizeof(WM_DELETE_MSG_ATOM_NAME) - 1, WM_DELETE_MSG_ATOM_NAME);
		xcb_intern_atom_cookie_t wmStateCookie = xcb_intern_atom(platformInfo.connection, false, sizeof(WM_STATE_ATOM_NAME) - 1, WM_STATE_ATOM_NAME);
		xcb_intern_atom_cookie_t wmMinimizedCookie = xcb_intern_atom(platformInfo.connection, false, sizeof(WM_STATE_ATOM_NAME) - 1, WM_STATE_ATOM_NAME);
		xcb_intern_atom_cookie_t wmMaximizedHCookie = xcb_intern_atom(platformInfo.connection, false, sizeof(WM_MAXIMIZED_H_ATOM_NAME) - 1, WM_MAXIMIZED_H_ATOM_NAME);
		xcb_intern_atom_cookie_t wmMaximizedVCookie = xcb_intern_atom(platformInfo.connection, false, sizeof(WM_MAXIMIZED_V_ATOM_NAME) - 1, WM_MAXIMIZED_V_ATOM_NAME);

		wmProtocolsAtom = xcb_intern_atom_reply(platformInfo.connection, wmProtocolsCookie, nullptr)->atom;
		wmDeleteMsgAtom = xcb_intern_atom_reply(platformInfo.connection, wmDeleteMsgCookie, nullptr)->atom;
		wmStateAtom = xcb_intern_atom_reply(platformInfo.connection, wmStateCookie, nullptr)->atom;
		wmMinimizedAtom = xcb_intern_atom_reply(platformInfo.connection, wmMinimizedCookie, nullptr)->atom;
		wmMaximizedHAtom = xcb_intern_atom_reply(platformInfo.connection, wmMaximizedHCookie, nullptr)->atom;
		wmMaximizedVAtom = xcb_intern_atom_reply(platformInfo.connection, wmMaximizedVCookie, nullptr)->atom;

		// Set the window's name
		xcb_change_property(platformInfo.connection, XCB_PROP_MODE_REPLACE, platformInfo.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(GSIM_PROJECT_NAME), GSIM_PROJECT_NAME);
		xcb_change_property(platformInfo.connection, XCB_PROP_MODE_REPLACE, platformInfo.window, wmProtocolsAtom, XCB_ATOM_ATOM, 32, 1, &wmDeleteMsgAtom);

		// Map the window to the screen
		xcb_map_window(platformInfo.connection, platformInfo.window);

		// Flush the stream
		if(xcb_flush(platformInfo.connection) <= 0)
			GSIM_LOG_FATAL("Failed to flush X stream!");

		GSIM_LOG_INFO("Created X window");
	}
	static void ProcessEvent(const xcb_generic_event_t* event) {
		switch(event->response_type & ~0x80) {
		case XCB_CONFIGURE_NOTIFY: {
			// Get the configure event pointer
			xcb_configure_notify_event_t* configureEvent = (xcb_configure_notify_event_t*)event;

			// Get the window's state and check if the window is minimized or maximized
			xcb_get_property_cookie_t wmStateCookie = xcb_get_property(platformInfo.connection, false, platformInfo.window, wmStateAtom, XCB_ATOM_ATOM, 0, 32);
			xcb_get_property_reply_t* wmStateReply = xcb_get_property_reply(platformInfo.connection, wmStateCookie, nullptr);
			xcb_atom_t wmNewState = *(xcb_atom_t*)xcb_get_property_value(wmStateReply);

			// Check if the window was just maximized or minimized
			bool maximized = (wmNewState == wmMaximizedHAtom) || (wmNewState == wmMaximizedVAtom);
			bool minimized = wmNewState == wmMinimizedAtom;

			// Set the move event info
			WindowMoveEventInfo moveInfo;

			moveInfo.newX = configureEvent->x;
			moveInfo.newY = configureEvent->y;

			// Set the resize event info
			WindowResizeEventInfo resizeInfo;

			resizeInfo.newWidth = configureEvent->width;
			resizeInfo.newHeight = configureEvent->height;
			resizeInfo.windowMaximized = !windowInfo.isMaximized && maximized;
			resizeInfo.windowUnmaximized = windowInfo.isMaximized && !maximized;
			resizeInfo.windowMinimized = !windowInfo.isMinimized && minimized;
			resizeInfo.windowUnminimized = windowInfo.isMinimized && !minimized;

			// Set the window's new info
			windowInfo.posX = moveInfo.newX;
			windowInfo.posY = moveInfo.newY;
			windowInfo.width = resizeInfo.newWidth;
			windowInfo.height = resizeInfo.newHeight;
			windowInfo.isMaximized = maximized;
			windowInfo.isMinimized = minimized;

			// Call the move and resize events
			moveEvent.CallEvent(&moveInfo);
			resizeEvent.CallEvent(&resizeInfo);

			break;
		}
		case XCB_CLIENT_MESSAGE: {
			// Get the client message event pointer
			xcb_client_message_event_t* clientMessageEvent = (xcb_client_message_event_t*)event;

			// Check if the window was closed
			if(clientMessageEvent->data.data32[0] == wmDeleteMsgAtom) {
				// Stop running the program
				running = false;
			}

			break;
		}
		}
	}

	// Public functions
	void CreateWindow() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		ConnectToX();
		CreateXWindow();
	}
	void DestroyWindow() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Disconnect from X
		xcb_disconnect(platformInfo.connection);
	}
	void PollWindowEvents() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Keep polling messages while running
		while(running) {
			// Process all pending events
			xcb_generic_event_t* event;
			while(running && (event = xcb_poll_for_event(platformInfo.connection)))
				ProcessEvent(event);
			
			// Exit the loop if the game is not running anymore
			if(!running)
				break;
			
			// Call the draw event
			drawEvent.CallEvent(nullptr);
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
		xcb_destroy_window(platformInfo.connection, platformInfo.window);

		// Stop running the game
		running = false;
	}
}

#endif