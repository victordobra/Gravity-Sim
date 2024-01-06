#include "ProgramLoop.hpp"
#include "Parser.hpp"
#include "Utils/Event.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/Renderer.hpp"
#include <time.h>
#include <stdio.h>

namespace gsim {
	// Variables
	static float deltaTime = 0.f;
	static float elapsedTime = 0.f;
#if defined(GSIM_PLATFORM_WINDOWS)
	static clock_t firstClock;
#elif defined(GSIM_PLATFORM_LINUX)
	static double firstSeconds;
#endif

	// Draw event callback
	static void* DrawEventCallback(void* params) {
		// Calculate the new elapsed time
#if defined(GSIM_PLATFORM_WINDOWS)
		float newElapsedTime = (float)(clock() - firstClock) / (float)CLOCKS_PER_SEC;
#elif defined(GSIM_PLATFORM_LINUX)
		timespec timespec;
		clock_gettime(CLOCK_REALTIME, &timespec);
		double currentSeconds = (double)timespec.tv_sec + (double)timespec.tv_nsec * 0.000000001;
		float newElapsedTime = (float)(currentSeconds - firstSeconds);
#endif
		// Calculate the delta time
		deltaTime = newElapsedTime - elapsedTime;
		elapsedTime = newElapsedTime;

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
#if defined(GSIM_PLATFORM_WINDOWS)
			// Set the first clock
			firstClock = clock();
#elif defined(GSIM_PLATFORM_LINUX)
			// Set the elapsed seconds at the current time
			timespec timespec;
			clock_gettime(CLOCK_REALTIME, &timespec);
			firstSeconds = (double)timespec.tv_sec + (double)timespec.tv_nsec * 0.000000001;
#endif

			// Bind the draw event
			GetWindowDrawEvent().AddListener(DrawEventCallback);
		} else {
			// Run the gravity simulation for one second at a time
			deltaTime = 1.f;

			for(elapsedTime = deltaTime; elapsedTime < GetSimulationDuration() + deltaTime; elapsedTime += deltaTime) {
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