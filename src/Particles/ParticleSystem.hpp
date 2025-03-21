#pragma once

#include "Particle.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A class defining a system of one or more particles.
	class ParticleSystem {
	public:
		/// @brief An enum containing all avaliable variants for particle system generation.
		enum GenerateType {
			/// @brief Randomly distribures particles within the generation confines.
			GENERATE_TYPE_RANDOM,
			/// @brief Simulates the approximate structure of a spiral galaxy spanning the generation confines.
			GENERATE_TYPE_GALAXY,
			/// @brief Simulates the collision of two sipral galaxies, each spanning a third of the generation confines.
			GENERATE_TYPE_GALAXY_COLLISION,
			/// @brief Simulates the collision of two symmetrical spiral galaxies, each spanning a third of the generation confines.
			GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION,
			/// @brief The number of possible particle system generation variants.
			GENERATE_TYPE_COUNT
		};
		/// @brief An enum containing all implemented simulation algorithms.
		enum SimulationAlgorithm {
			/// @brief The direct-sum method, calculating every interaction between particles.
			SIMULATION_ALGORITHM_DIRECT_SUM,
			/// @brief The Barnes-Hut algorithm, organizing all particles in a quadtree.
			SIMULATION_ALGORITHM_BARNES_HUT,
			/// @brief The number of implemented simulation algorithms.
			SIMULATION_ALGORITHM_COUNT
		};
		/// @brief A struct containing all buffers for the particle infos.
		struct ParticleBuffers {
			/// @brief A buffer storing the particle positions.
			VkBuffer posBuffer;
			/// @brief A buffer storing the particle velocities.
			VkBuffer velBuffer;
			/// @brief A buffer storing the particle masses.
			VkBuffer massBuffer;
		};

		ParticleSystem() = delete;
		ParticleSystem(const ParticleSystem&) = delete;
		ParticleSystem(ParticleSystem&&) noexcept = delete;

		/// @brief Loads a particle system from the given file.
		/// @param device The Vulkan device to use for Vulkan-specific components.
		/// @param filePath The path of the file to load the system from.
		/// @param gravitationalConst The gravitational constant used for the simulation.
		/// @param simulationTime The time interval length, in seconds, simulated in one instance.
		/// @param simulationSpeed The speed factor at which the simulation is run.
		/// @param softeningLen The softening length used to soften the extreme forces that would usually result from close interactions.
		/// @param accuracyParameter The accuracy parameter used to calibrate force approximation. Only used for Barnes-Hut simulations.
		/// @param simulationAlgorithm The simulation algorithm used to calculate the gravitational forces.
		ParticleSystem(VulkanDevice* device, const char* filePath, float gravitationalConst, float simulationTime, float simulationSpeed, float softeningLen, float accuracyParameter, SimulationAlgorithm simulationAlgorithm);
		/// @brief Generates a particle system based on the given parameters.
		/// @param device The Vulkan device to use for Vulkan-specific components.
		/// @param particleCount The number of particles in the system.
		/// @param generateType The variant to use for the system generation.
		/// @param generateSize The radius of the resulting generation's size.
		/// @param minMass The minimum possible value of the particles' mass.
		/// @param maxMass The maximum possible value of the particles' mass.
		/// @param gravitationalConst The gravitational constant used for the simulation.
		/// @param simulationTime The time interval length, in seconds, simulated in one instance.
		/// @param simulationSpeed The speed factor at which the simulation is run.
		/// @param softeningLen The softening length used to soften the extreme forces that would usually result from close interactions.
		/// @param accuracyParameter The accuracy parameter used to calibrate force approximation. Only used for Barnes-Hut simulations.
		/// @param simulationAlgorithm The simulation algorithm used to calculate the gravitational forces.
		ParticleSystem(VulkanDevice* device, size_t particleCount, GenerateType generateType, float generateSize, float minMass, float maxMass, float gravitationalConst, float simulationTime, float simulationSpeed, float softeningLen, float accuracyParameter, SimulationAlgorithm simulationAlgorithm);

		ParticleSystem& operator=(const ParticleSystem&) = delete;
		ParticleSystem& operator=(ParticleSystem&&) noexcept = delete;

		/// @brief Gets the Vulkan device that the particle system uses.
		/// @return A pointer to the Vulkan device wrapper object.
		VulkanDevice* GetDevice() {
			return device;
		}
		/// @brief Gets the Vulkan device that the particle system uses.
		/// @return A const pointer to the Vulkan device wrapper object.
		const VulkanDevice* GetDevice() const {
			return device;
		}

		/// @brief Gets the number of particles in the system.
		/// @return The number of particles in the system.
		size_t GetParticleCount() const {
			return particleCount;
		}
		/// @brief Gets the number of particles in the system, aligned for easy usage by the simulation.
		/// @return The number of particles in the system, aligned for easy usage by the simulation.
		size_t GetAlignedParticleCount() const {
			return alignedParticleCount;
		}
		/// @brief Gets the number of particles that the particle buffers have room for. Larger for Barnes-Hut simulations.
		/// @return The number of particles that the particle buffers have room for.
		size_t GetBufferSize() const {
			return bufferSize;
		}
		/// @brief Gets the gravitational constant used for the simulation.
		/// @return The gravitational constant used for the simulation.
		float GetGravitationalConst() const {
			return gravitationalConst;
		}
		/// @brief Gets the time interval length, in seconds, simulated in one instance.
		/// @return The time interval length, in seconds, simulated in one instance.
		float GetSimulationTime() const {
			return simulationTime;
		}
		/// @brief Gets the speed factor at which the simulation is run.
		/// @return The speed factor at which the simulation is run.
		float GetSimulationSpeed() const {
			return simulationSpeed;
		}
		/// @brief Gets the softening length used to soften the extreme forces that would usually result from close interactions.
		/// @return The softening length used to soften the extreme forces that would usually result from close interactions.
		float GetSofteningLen() const {
			return softeningLen;
		}
		/// @brief Gets the accuracy parameter used to calibrate force approximation.
		/// @return The accuracy parameter used to calibrate force approximation.
		float GetAccuracyParameter() const {
			return accuracyParameter;
		}

		/// @brief Gets the camera's starting position.
		/// @return The camera's starting position.
		Vec2 GetCameraStartPos() const {
			return cameraStartPos;
		}
		/// @brief Gets the camera's starting size.
		/// @return The camera's starting size.
		float GetCameraStartSize() const {
			return cameraStartSize;
		}

		/// @brief Gets the Vulkan buffers storing the particle infos.
		/// @return A pointer to the array of particle buffers.
		ParticleBuffers* GetBuffers() {
			return buffers;
		}
		/// @brief Gets the Vulkan device memory block the buffers are bound to.
		/// @return The Vulkan device memory block the buffers are bound to.
		VkDeviceMemory GetBufferMemory() {
			return bufferMemory;
		}

		/// @brief Gets the index of the particle buffer to use for graphics.
		/// @return The index of the particle buffer to use for graphics.
		size_t GetGrahpicsIndex() const {
			return graphicsIndex;
		}
		/// @brief Gets the index of the particle buffer to input for computations.
		/// @return The index of the particle buffer to input for computations.
		size_t GetComputeInputIndex() const {
			return computeInputIndex;
		}
		/// @brief Gets the index of the particle buffer in which computation outputs will be stored.
		/// @return The index of the particle buffer in which computation outputs will be stored.
		size_t GetComputeOutputIndex() const {
			return computeOutputIndex;
		}
		/// @brief Saves the index of the next buffer to use for graphics.
		void NextGraphicsIndex() {
			// Arrange the indices to keep the simulations up-to-date and the graphics only one simulation behind
			size_t aux = graphicsIndex;
			graphicsIndex = computeOutputIndex;
			computeOutputIndex = aux;
		}
		/// @brief Saves the indices of the next buffers to use for computations.
		void NextComputeIndices() {
			// Swap the two compute buffer indices
			size_t aux = computeInputIndex;
			computeInputIndex = computeOutputIndex;
			computeOutputIndex = aux;
		}
	
		/// @brief Gets the system's partile infos.
		/// @param particles A pointer to the array in which the particle infos will be written.
		void GetParticles(Particle* particles);
		/// @brief Saves the system's particle infos to a file.
		/// @param filePath The path of the file to save the infos to.
		void SaveParticles(const char* filePath);

		/// @brief Destroys the particle system.
		~ParticleSystem();
	private:
		void GenerateParticlesRandom(Particle* particles, float generateSize, float minMass, float maxMass);
		void GenerateParticlesGalaxy(Particle* particles, float generateSize, float minMass, float maxMass);
		void GenerateParticlesGalaxyCollision(Particle* particles, float generateSize, float minMass, float maxMass);
		void GenerateParticlesSymmetricalGalaxyCollision(Particle* particles, float generateSize, float minMass, float maxMass);
		void CreateVulkanObjects(const Particle* particles);
		void GetCameraInfo(const Particle* particles);

		VulkanDevice* device;

		size_t particleCount;
		size_t alignedParticleCount;
		size_t bufferSize;
		float gravitationalConst;
		float simulationTime;
		float simulationSpeed;
		float softeningLen;
		float accuracyParameter;

		Vec2 cameraStartPos;
		float cameraStartSize;
		
		ParticleBuffers buffers[3];
		VkDeviceMemory bufferMemory;

		size_t graphicsIndex = 0;
		size_t computeInputIndex = 1;
		size_t computeOutputIndex = 2;
	};
}