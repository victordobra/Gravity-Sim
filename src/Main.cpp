#include "ProjectInfo.hpp"
#include "Debug/Exception.hpp"
#include "Debug/Logger.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/VulkanCommandPool.hpp"
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
		gsim::Window* window = new gsim::Window(GSIM_PROJECT_NAME, 512, 512);

		// Create the Vulkan components
		gsim::VulkanInstance* instance = new gsim::VulkanInstance(true, &logger);
		gsim::VulkanSurface* surface = new gsim::VulkanSurface(instance, window);
		gsim::VulkanDevice* device = new gsim::VulkanDevice(instance, surface);
		gsim::VulkanCommandPool* graphicsCommandPool = new gsim::VulkanCommandPool(device, device->GetQueueFamilyIndices().graphicsIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		gsim::VulkanCommandPool* transferCommandPool = new gsim::VulkanCommandPool(device, device->GetQueueFamilyIndices().transferIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		gsim::VulkanCommandPool* computeCommandPool = new gsim::VulkanCommandPool(device, device->GetQueueFamilyIndices().computeIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		gsim::VulkanSwapChain* swapChain = new gsim::VulkanSwapChain(device, surface);

		// Parse the window's events, as long as it is running
		while(window->GetWindowInfo().running) {
			window->ParseEvents();
		}

		// Destroy the Vulkan components
		delete swapChain;
		delete graphicsCommandPool;
		delete transferCommandPool;
		delete computeCommandPool;
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