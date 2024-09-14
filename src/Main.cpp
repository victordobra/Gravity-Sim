#include "ProjectInfo.hpp"
#include "Debug/Exception.hpp"
#include "Debug/Logger.hpp"
#include "Graphics/GraphicsPipeline.hpp"
#include "Particles/Particle.hpp"
#include "Particles/ParticleSystem.hpp"
#include "Platform/Window.hpp"
#include "Simulation/Direct/DirectSimulation.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanSurface.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include <stdint.h>
#include <time.h>
#include <exception>

struct WindowDrawData {
	gsim::GraphicsPipeline* graphicsPipeline;
	gsim::DirectSimulation* simulation;

	clock_t clockStart;
	uint64_t simulationCount;
	uint32_t targetSimulationCount;
};

static void WindowDrawCallback(void* userData, void* args) {
	// Get the window draw data
	WindowDrawData* drawData = (WindowDrawData*)userData;

	// Run the simulations
	drawData->simulation->RunSimulations(drawData->targetSimulationCount - drawData->simulationCount);
	drawData->simulationCount = drawData->targetSimulationCount;

	// Render the particles
	drawData->graphicsPipeline->RenderParticles();

	// Store the clock end and calculate the target simulation count
	drawData->targetSimulationCount = (uint64_t)((float)(clock() - drawData->clockStart) / CLOCKS_PER_SEC / .001f);
}

int main(int argc, char** args) {
	// Create the logger
	gsim::Logger logger("log.txt", gsim::Logger::MESSAGE_LEVEL_ALL);

	// Catch any exceptions thrown by the rest of the program
	try {
		// Create the window
		gsim::Window* window = new gsim::Window(GSIM_PROJECT_NAME, 800, 800);

		// Create the Vulkan components
		gsim::VulkanInstance* instance = new gsim::VulkanInstance(true, &logger);
		gsim::VulkanSurface* surface = new gsim::VulkanSurface(instance, window);
		gsim::VulkanDevice* device = new gsim::VulkanDevice(instance, surface);
		gsim::VulkanSwapChain* swapChain = new gsim::VulkanSwapChain(device, surface);
	
		// Log info about the Vulkan objects
		device->LogDeviceInfo(&logger);
		swapChain->LogSwapChainInfo(&logger);

		// Create the particle system
		gsim::ParticleSystem* particleSystem = new gsim::ParticleSystem(device, 10000, gsim::ParticleSystem::GENERATE_TYPE_GALAXY_COLLISION, 200, 1, 5, 500, 1, .001f, .2f);

		// Create the pipelines
		gsim::GraphicsPipeline* graphicsPipeline = new gsim::GraphicsPipeline(device, swapChain, particleSystem);
		gsim::DirectSimulation* simulation = new gsim::DirectSimulation(device, particleSystem);

		// Set the window draw data
		WindowDrawData drawData {
			.graphicsPipeline = graphicsPipeline,
			.simulation = simulation,
			.clockStart = clock(),
			.simulationCount = 0,
			.targetSimulationCount = 0
		};

		// Add the window draw listener
		window->GetDrawEvent().AddListener({ WindowDrawCallback, &drawData });

		while(window->GetWindowInfo().running) {
			// Parse the window's events
			window->ParseEvents();

			// Run the simulations
			simulation->RunSimulations(drawData.targetSimulationCount - drawData.simulationCount);
			drawData.simulationCount = drawData.targetSimulationCount;

			// Store the clock end and calculate the target simulation count
			drawData.targetSimulationCount = (uint64_t)((float)(clock() - drawData.clockStart) / CLOCKS_PER_SEC / .001f);
		}

		// Destroy the pipelines
		delete graphicsPipeline;
		delete simulation;

		// Destroy the particle system
		delete particleSystem;

		// Destroy the Vulkan components
		delete swapChain;
		delete device;
		delete surface;
		delete instance;

		// Destroy the window
		delete window;
	} catch(const gsim::Exception& exception) {
		// Log the exception
		logger.LogException(exception);
	} catch(const std::exception& exception) {
		// Log the exception
		logger.LogStdException(exception);
	}

	return 0;
}