#include "Manager/Parser.hpp"
#include "Debug/Logger.hpp"
#include "Platform/Window.hpp"
#include "Vulkan/Renderer.hpp"
#include <unistd.h>

int main(int argc, char** args) {
	// Try to parse the console arguments
	if(!gsim::ParseArgs(argc, args))
		return 0;

	// Create every component
	gsim::CreateLogger();
	gsim::CreateWindow();
	gsim::CreateVulkanRenderer();

	// Run the main program loop
	while(gsim::PollWindowEvents()) {
		// Placeholder
		sleep(0.01);
	}

	// Destroy every component
	gsim::DestroyVulkanRenderer();
	gsim::DestroyWindow();
	gsim::DestroyLogger();

	return 0;
}