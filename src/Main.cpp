#include "Platform/Window.hpp"
#include <unistd.h>

int main(int argc, char** args) {
	// Create every component
	gsim::CreateWindow();

	// Run the main program loop
	while(gsim::PollWindowEvents()) {
		// Placeholder
		sleep(0.05);
	}

	// Destroy every component
	gsim::DestroyWindow();

	return 0;
}