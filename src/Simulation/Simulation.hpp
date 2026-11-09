#pragma once

#include <stdint.h>

namespace gsim {
	/// @brief The simulation interface class used for all simulation implementations.
	class Simulation {
	public:
		/// @brief Creates a simulation.
		Simulation() = default;
		/// @brief Copies a simulation.
		/// @param other The simulation to copy.
		Simulation(const Simulation& other) = default;
		/// @brief Moves a simulation.
		/// @param other The simulation to move.
		Simulation(Simulation&& other) noexcept = default;

		/// @brief Copies a simulation.
		/// @param other The simulation to copy.
		/// @return A reference to this simulation.
		Simulation& operator=(const Simulation& other) = default;
		/// @brief Moves a simulation.
		/// @param other The simulation to move.
		/// @return A reference to this simulation.
		Simulation& operator=(Simulation&& other) noexcept = default;

		/// @brief Runs the given number of simulations.
		/// @param simulationCount The number of simulations to run.
		virtual void RunSimulations(uint32_t simulationCount) = 0;

		/// @brief Destroys this simulation.
		virtual ~Simulation() = default;
	};
}