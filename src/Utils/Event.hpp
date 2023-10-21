#pragma once

#include <vector>
#include <stdint.h>

namespace gsim {
    /// @brief A class implementing an event with multiple listeners.
    class Event {
    public:
        /// @brief A function pointer for event listeners.
        typedef void*(*Listener)(void* params);

        /// @brief Creates an event with no listeners.
        Event() = default;
        Event(const Event&) = delete;
        Event(Event&) noexcept = delete;

        Event& operator=(const Event&) = delete;
        Event& operator=(Event&&) noexcept = delete;

        /// @brief Adds a listener for the event.
        /// @param listener The listener to add for the event.
        void AddListener(Listener listener);
        /// @brief Gets the number of listeners for the event.
        /// @return The number of listeners.
        size_t GetListenerCount() const;
        /// @brief Gets the list of listeners for the event.
        /// @return A pointer to the array of listeners.
        Listener* GetListeners();
        /// @brief Gets the list of listeners for the event.
        /// @return A const pointer to the array of listeners.
        const Listener* GetListeners() const;

        /// @brief Triggers the event's calling all listener callbacks.
        /// @param params The params to be used when calling the listener callbacks.
        /// @param returns A pointer to an array where the function will store the returns of the 
        void TriggerEvent(void* params, void** returns = nullptr);

        /// @brief Destroys the event.
        ~Event() = default;
    private:
        std::vector<Listener> listeners{};
    };    
}