#include "ProgramLoop.hpp"
#include "Parser.hpp"
#include "Utils/Event.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/Renderer.hpp"
#include <time.h>

namespace gsim {
	// Variables
	static float deltaTime = 0.f;
	static float elapsedTime = 0.f;
	static clock_t lastClock;

	// Draw event callback
	static void* DrawEventCallback(void* params) {
		// Calculate the delta time
		deltaTime = (float)(clock() - lastClock) / (float)CLOCKS_PER_SEC;
		elapsedTime += deltaTime;

		// Set the new last clock
		lastClock = clock();

		// Wait for the previous frame to finish execution
		WaitForVulkanFences();

		// Simulate graivty
		SimulateGravity();

		// Draw the points
		DrawPoints();

		// Close the window if the elapsed time surpasses the simulation duration
		if(elapsedTime > GetSimulationDuration())
			CloseWindow();

		return nullptr;
	}

	void StartProgramLoop() {
		if(IsRenderingEnabled()) {
			// Set the last clock
			lastClock = clock();

			// Bind the draw event
			GetWindowDrawEvent().AddListener(DrawEventCallback);
		} else {
			for(; elapsedTime <= GetSimulationDuration(); elapsedTime += 1.f) {
				// Wait for the previous frame to finish execution
				WaitForVulkanFences();

				// Simulate gravity
				SimulateGravity();
			}
		}
	}

	float GetSimulationDeltaTime() {
		return deltaTime;
	}
	float GetSimulationElapsedTime() {
		return elapsedTime;
	}
}