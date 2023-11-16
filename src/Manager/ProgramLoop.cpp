#include "ProgramLoop.hpp"
#include "Parser.hpp"
#include "Utils/Event.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/Renderer.hpp"
#include <time.h>

namespace gsim {
	// Variables
	static double deltaTime = 0.0;
	static double elapsedTime = 0.0;
	static clock_t lastClock;

	// Draw event callback
	static void* DrawEventCallback(void* params) {
		// Calculate the delta time
		deltaTime = (double)(clock() - lastClock) / (double)CLOCKS_PER_SEC;
		elapsedTime += deltaTime;

		// Set the new last clock
		lastClock = clock();

		// Draw the points
		DrawPoints();

		// Close the window if the elapsed time surpasses the simulation duration
		if(elapsedTime > GetSimulationDuration())
			CloseWindow();

		return nullptr;
	}

	void StartProgramLoop() {
		// Set the last clock
		lastClock = clock();

		// Bind the draw event
		GetWindowDrawEvent().AddListener(DrawEventCallback);
	}
}