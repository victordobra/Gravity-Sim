#include "Platform/Window.hpp"
#include "Vulkan/Renderer.hpp"
#include <unistd.h>

int main(int argc, char** args) {
	// Create every component
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

	return 0;
}