#include "ProjectInfo.hpp"
#include "Debug/Exception.hpp"
#include "Debug/Logger.hpp"
#include "Graphics/GraphicsPipeline.hpp"
#include "Particles/Particle.hpp"
#include "Particles/ParticleSystem.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanSurface.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include <stdint.h>
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
		gsim::ParticleSystem* particleSystem = new gsim::ParticleSystem(device, 2048, gsim::ParticleSystem::GENERATE_TYPE_GALAXY, 200, 1, 100, 500, 1, .0001f, .1f);

		// Create the pipelines
		gsim::GraphicsPipeline* graphicsPipeline = new gsim::GraphicsPipeline(device, swapChain, particleSystem);

		// Run the window's loop
		while(window->GetWindowInfo().running) {
			// Parse the window's events
			window->ParseEvents();

			// Render the particles
			graphicsPipeline->RenderParticles();
		}

		// Destroy the pipelines
		delete graphicsPipeline;

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