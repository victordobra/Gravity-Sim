#include "Parser.hpp"
#include "ProjectInfo.hpp"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace gsim {
	// Internal variables with default values set
	size_t pointCount;
	const char* pointInFileName = nullptr;
	const char* pointOutFileName = nullptr;
	const char* logFileName = nullptr;
	double simulationInterval = 0.001;
	double gravitationalConstant = 1;
	double simulationDuration = INFINITY;

	bool isVerbose = false;
	bool isValidationEnabled = false;

	// Internal helper functions
	static void WriteVersion() {
		// Write the version
		printf("%s, version %u.%u.%u\n", GSIM_PROJECT_NAME, GSIM_PROJECT_VERSION_MAJOR, GSIM_PROJECT_VERSION_MINOR, GSIM_PROJECT_VERSION_PATCH);
	}
	static void WriteHelp() {
		// Write the help
		printf("Available parameters:\n"
		"--point-count: The number of points to simulate. Used only if --points-in is not set, in which the given number of points will start in a circular arrangement moving towards the center with random masses.\n"
		"--points-in: The file containing all points to be simulated. Must be set if --point-count is not set.\n"
		"--points-out: The file in which the final coordinates of the points will be written. Optional.\n"
		"--log-file: The file in which all program logs will be stored. Optional.\n"
		"--sim-interval: The time interval, in seconds, to use for calculating gravitational force. Defaulted to 1e-3.\n"
		"--gravitational-const: The gravitation constant to use for calculating gravitational force. Defaulted to 1.\n"
		"--simulation-duration: The duration, in seconds, of the simulation. Defaulted to infinity.\n\n"
		"Available options:\n"
		"--version: Prints the program's version and exits the program.\n"
		"--help: Prints the current message and exits the program.\n"
		"--verbose: Prints additional debug messages about the state of the program.\n"
		"--vulkan-validation: Enables Vulkan validation for the rest of the program's runtime.\n");
	}

	// Public functions
	bool ParseArgs(int argc, char** args) {
		// Write the program's version if the first argument is --version
		if(argc == 2 && !strcmp(args[1], "--version")) {
			WriteVersion();
			return false;
		}

		// Write the requested help if the first argument is --help
		if(argc == 2 && !strcmp(args[1], "--help")) {
			WriteHelp();
			return false;
		}

		// Read the given args
		for(int32_t i = 1; i != argc; ++i) {
			// Check if the given argument is the name of a parameter
			if(!strcmp(args[i], "--point-count")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Convert the current parameter to an integer
				pointCount = (size_t)strtoull(args[i], nullptr, 10);
				if(pointCount == 0) {
					printf("Invalid args: invalid point count!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				continue;
			}
			if(!strcmp(args[i], "--points-in")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Save the file name
				pointInFileName = args[i];

				continue;
			}
			if(!strcmp(args[i], "--points-out")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Save the file name
				pointOutFileName = args[i];

				continue;
			}
			if(!strcmp(args[i], "--log-file")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Save the file name
				logFileName = args[i];

				continue;
			}
			if(!strcmp(args[i], "--sim-interval")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Convert the current argument to a double
				simulationInterval = strtod(args[i], nullptr);
				if(simulationInterval == 0) {
					printf("Invalid args: invalid simulation interval!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				continue;
			}
			if(!strcmp(args[i], "--gravitational-const")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Convert the current argument to a double
				gravitationalConstant = strtod(args[i], nullptr);
				if(gravitationalConstant == 0) {
					printf("Invalid args: invalid gravitational constant!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				continue;
			}
			if(!strcmp(args[i], "--simulation-duration")) {
				// Check if this is the last argument
				if(++i == argc) {
					printf("Invalid args: parameter listed, but no value given!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				// Convert the current argument to a double
				simulationDuration = strtod(args[i], nullptr);
				if(simulationDuration == 0) {
					printf("Invalid args: invalid simulation duration!\nUse --help for a detailed list of all parameters and options.\n");
					return false;
				}

				continue;
			}

			// Check if the current parameter is the name of an option
			if(!strcmp(args[i], "--verbose")) {
				// Enable verbose logging
				isVerbose = true;

				continue;
			}
			if(!strcmp(args[i], "--vulkan-validation")) {
				// Enable Vulkan validation
				isValidationEnabled = true;

				continue;
			}

			// No valid parameter or option name was given; print an error and exit the function
			printf("Invald args: invalid parameter or option name!\nUse --help for a detailed list of all parameters and options.\n");
			return false;
		}
		
		// Check if a point input file or the point count was given
		if(!pointInFileName && !pointCount) {
			printf("Invalid args: no point input file name was given!\nUse --help for a detailed list of all parameters and options.\n");
			return false;
		}

		return true;
	}

	size_t GetArgsPointCount() {
		return pointCount;
	}
	const char* GetPointInFileName() {
		return pointInFileName;
	}
	const char* GetPointOutFileName() {
		return pointOutFileName;
	}
	const char* GetLogFileName() {
		return logFileName;
	}
	double GetSimulationInterval() {
		return simulationInterval;
	}
	double GetGravitationalConstant() {
		return gravitationalConstant;
	}
	double GetSimulationDuration() {
		return simulationDuration;
	}

	bool IsVerbose() {
		return isVerbose;
	}
	bool IsValidationEnabled() {
		return isValidationEnabled;
	}
}