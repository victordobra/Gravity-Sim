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

		// Create the particle system
		gsim::ParticleSystem* particleSystem = new gsim::ParticleSystem(device, 12000, gsim::ParticleSystem::GENERATE_TYPE_GALAXY_COLLISION, 200, 10, 100, 500, .5f, .001f, .3f);

		// Create the pipelines
		gsim::GraphicsPipeline* graphicsPipeline = new gsim::GraphicsPipeline(device, swapChain, particleSystem);
		gsim::DirectSimulation* simulation = new gsim::DirectSimulation(device, particleSystem);

		// Run the window's loop
		uint64_t simulationCount = 0, targetSimulationCount = 0;
		clock_t clockStart = clock();

		while(window->GetWindowInfo().running) {
			// Parse the window's events
			window->ParseEvents();

			// Run the simulations
			simulation->RunSimulations(targetSimulationCount - simulationCount);
			simulationCount = targetSimulationCount;

			// Render the particles
			graphicsPipeline->RenderParticles();

			// Store the clock end and calculate the target simulation count
			clock_t clockEnd = clock();
			targetSimulationCount = (uint64_t)((float)clock() / CLOCKS_PER_SEC / .001f);
			clockStart = clockEnd;
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