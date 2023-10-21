#include "Event.hpp"

namespace gsim {
	void Event::AddListener(Listener listener) {
		// Add the listener to the vector
		listeners.push_back(listener);
	}
	size_t Event::GetListenerCount() const {
		// Return the size of the listener vector
		return listeners.size();
	}
	Event::Listener* Event::GetListeners() {
		// Return the data of the listener vector
		return listeners.data();
	}
	const Event::Listener* Event::GetListeners() const {
		// Return the data of the listener vector
		return listeners.data();
	}

	void Event::CallEvent(void* params, void** returns) {
		// Loop through every listener
		for(Listener listener : listeners) {
			// Trigger the listener callback
			void* returnVal = listener(params);

			// Save the return value to the vector, if it exists
			if(returns)
				*returns++ = returnVal;
		}
	}
}