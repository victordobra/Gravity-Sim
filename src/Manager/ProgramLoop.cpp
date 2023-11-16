#include "ProgramLoop.hpp"
#include "Utils/Event.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/Renderer.hpp"

namespace gsim {
    // Draw event callback
    static void* DrawEventCallback(void* params) {
        // Draw the points
        DrawPoints();
    }

    void StartProgramLoop() {
        // Bind the draw event
        GetWindowDrawEvent().AddListener(DrawEventCallback);
    }
}