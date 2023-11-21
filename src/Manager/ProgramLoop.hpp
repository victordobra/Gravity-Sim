#pragma once

namespace gsim {
	/// @brief Starts the main program loop.
	void StartProgramLoop();

	/// @brief Gets the ammount of time the previous simulation took.
	/// @return The ammount of time, in seconds, the previous simulation took.
	double GetSimulationDeltaTime();
	/// @brief Gets the ammount of time since the simulation started.
	/// @return The amount of time, in seconds, since the simulation started.
	double GetSimulationElapsedTime();
}