#pragma once

#include "Debug/Exception.hpp"
#include <stdint.h>

namespace gsim {
	/// @brief An event class that calls multiple listener functions when requested.
	class Event {
	public:
		/// @brief An event listener callback. 
		typedef void(*ListenerCallback)(void* userData, void* args);

		/// @brief A struct containing the info for an event listener.
		struct Listener {
			/// @brief The callback for the listener.
			ListenerCallback callback;
			/// @brief The data to be passed to the callback as a parameter.
			void* userData;
		};

		/// @brief Creates an event with no listeners.
		Event() = default;
		/// @brief Copies the given event's info.
		/// @param other The event to copy from.
		Event(const Event& other) = default;
		/// @brief Moves the given event's info.
		/// @param other The event to move from.
		Event(Event&& other) noexcept = default;

		/// @brief Copies the given event's info into this event.
		/// @param other The event to copy from.
		/// @return A reference to this event.
		Event& operator=(const Event& other) = default;
		/// @brief Moves the given event's info into this event.
		/// @param other The event to move from.
		/// @return A reference to this event.
		Event& operator=(Event&& other) = default;

		/// @brief Gets the number of listeners.
		/// @return The number of listeners.
		size_t GetListenerCount() const {
			return listenerCount;
		}
		/// @brief Gets the event's listeners.
		/// @return A pointer to the array of listeners.
		Listener* GetListeners() {
			return listeners;
		}
		/// @brief Gets the event's listeners.
		/// @return A const pointer to the array of listeners.
		const Listener* GetListeners() const {
			return listeners;
		}

		/// @brief Adds the given listener to the event.
		/// @param listener The listener to add to the event.
		void AddListener(Listener listener) {
			// Check if the listener array is full
			if(listenerCount == MAX_LISTENER_COUNT)
				GSIM_THROW_EXCEPTION("Event listener array is already full, no other listener can be added!");
			
			// Add the listener to the array
			listeners[listenerCount++] = listener;
		}
		/// @brief Removes the listener at the given index from the event.
		/// @param index The index in the listener array of the listener to remove.
		void RemoveListener(size_t index) {
			// Move all following elements back
			for(size_t i = index; i != listenerCount - 1; ++i)
				listeners[i] = listeners[i + 1];
			
			// Decrement the listener count
			--listenerCount;
		}

		/// @brief Calls every listener of the event.
		/// @param args The args to be passed down to the listener callbacks.
		void CallEvent(void* args) {
			// Call every listener's callback with the given args
			for(size_t i = 0; i != listenerCount; ++i)
				listeners[i].callback(listeners[i].userData, args);
		}

		/// @brief Destroys this event.
		~Event() = default;
	private:
		static const size_t MAX_LISTENER_COUNT = 64;

		size_t listenerCount = 0;
		Listener listeners[MAX_LISTENER_COUNT];
	};
}