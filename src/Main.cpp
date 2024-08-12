#include "ProjectInfo.hpp"
#include "Debug/Exception.hpp"
#include "Debug/Logger.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanSurface.hpp"

#include <stdint.h>
#include <exception>

int main(int argc, char** args) {
	// Create the logger
	gsim::Logger logger("log.txt", gsim::Logger::MESSAGE_LEVEL_ALL);

	// Catch any exceptions thrown by the rest of the program
	try {
		// Create the window
		gsim::Window* window = new gsim::Window(GSIM_PROJECT_NAME, 512, 512);

		// Create the Vulkan components
		gsim::VulkanInstance* instance = new gsim::VulkanInstance(true, &logger);
		gsim::VulkanSurface* surface = new gsim::VulkanSurface(instance, window);
		gsim::VulkanDevice* device = new gsim::VulkanDevice(instance, surface);

		// Parse the window's events, as long as it is running
		while(window->GetWindowInfo().running) {
			window->ParseEvents();
		}

		// Destroy the Vulkan components
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