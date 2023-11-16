#include "Manager/Parser.hpp"
#include "Manager/ProgramLoop.hpp"
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

	// Start the program loop and keep polling platform events until the window is closed
	gsim::StartProgramLoop();
	gsim::PollWindowEvents();

	// Destroy every component
	gsim::DestroyVulkanRenderer();
	gsim::DestroyWindow();
	gsim::DestroyLogger();

	return 0;
}