#pragma once

#include <stdint.h>

namespace gsim {
	/// @brief Parses the command line arguments given to the program.
	/// @param argc The number of command line arguments.
	/// @param args The command line arguments.
	/// @return True if the command line args are valid, otherwise false.
	bool ParseArgs(int argc, char** args);

	/// @brief Gets the name of the file containing all point data.
	/// @return The name of the file containing all point data.
	const char* GetPointInFileName();
	/// @brief Gets the name of the file in which the final positions of the points will be written.
	/// @return The name of the file in which the final positions of the points will be written, or a nullptr if no such file was given.
	const char* GetPointOutFileName();
	/// @brief Gets the name of the file in which all program logs will be stored.
	/// @return The name of the file in which all program logs will be stored.
	const char* GetLogFileName();
	/// @brief Gets the duration used for calculating gravitational force.
	/// @return The time interval used for calculating gravitational force.
	double GetSimulationInterval();
	/// @brief Gets the gravitational constant used for calculation gravitational force.
	/// @return The gravitational constant used for calculation gravitational force.
	double GetGravitationalConstant();
	/// @brief Gets the time limit of the simulation.
	/// @return The time limit of the simulation, or INFINITY if no limit was given.
	double GetSimulationDuration();

	/// @brief Checks if verbose logging is enabled.
	/// @return True if verbose logging is enabled, otherwise false.
	bool IsVerbose();
	/// @brief Checks if Vulkan validation is enabled.
	/// @return True if Vulkan validation is enabled, otherwise false.
	bool IsValidationEnabled();
}