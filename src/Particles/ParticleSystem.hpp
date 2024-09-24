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

		ParticleSystem() = delete;
		ParticleSystem(const ParticleSystem&) = delete;
		ParticleSystem(ParticleSystem&&) noexcept = delete;

		/// @brief Loads a particle system from the given file.
		/// @param device The Vulkan device to use for Vulkan-specific components.
		/// @param filePath The path of the file to load the system from.
		/// @param systemSize The side length of the system's bounding box.
		/// @param gravitationalConst The gravitational constant used for the simulation.
		/// @param simulationTime The time interval length, in seconds, simulated in one instance.
		/// @param softeningLen The softening length used to soften the extreme forces that would usually result from close interactions.
		/// @param particleCountAlignment The alignment to use for the particle count when creating the buffer. All extra particles are given a null mass, therefore they will not interfere with the simulation. Defaulted to 1.
		ParticleSystem(VulkanDevice* device, const char* filePath, float systemSize, float gravitationalConst, float simulationTime, float softeningLen, size_t particleCountAlignment = 1);
		/// @brief Generates a particle system based on the given parameters.
		/// @param device The Vulkan device to use for Vulkan-specific components.
		/// @param particleCount The number of particles in the system.
		/// @param generateType The variant to use for the system generation.
		/// @param generateSize The radius of the resulting generation's size.
		/// @param minMass The minimum possible value of the particles' mass.
		/// @param maxMass The maximum possible value of the particles' mass.
		/// @param systemSize The side length of the system's bounding box.
		/// @param gravitationalConst The gravitational constant used for the simulation.
		/// @param simulationTime The time interval length, in seconds, simulated in one instance.
		/// @param softeningLen The softening length used to soften the extreme forces that would usually result from close interactions.
		/// @param particleCountAlignment The alignment to use for the particle count when creating the buffer. All extra particles are given a null mass, therefore they will not interfere with the simulation. Defaulted to 1.
		ParticleSystem(VulkanDevice* device, size_t particleCount, GenerateType generateType, float generateSize, float minMass, float maxMass, float systemSize, float gravitationalConst, float simulationTime, float softeningLen, size_t particleCountAlignment = 1);

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
		/// @brief Gets the side length of the system's bounding box.
		/// @return The side length of the system's bounding box.
		float GetSystemSize() const {
			return systemSize;
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
		/// @brief Gets the softening length used to soften the extreme forces that would usually result from close interactions.
		/// @return The softening length used to soften the extreme forces that would usually result from close interactions.
		float GetSofteningLen() const {
			return softeningLen;
		}

		/// @brief Gets the Vulkan buffers storing the particle infos.
		/// @return A pointer to the array of buffers.
		VkBuffer* GetBuffers() {
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

		VulkanDevice* device;

		size_t particleCount;
		size_t alignedParticleCount;
		float systemSize;
		float gravitationalConst;
		float simulationTime;
		float softeningLen;
		
		VkBuffer buffers[3];
		VkDeviceMemory bufferMemory;

		size_t graphicsIndex = 0;
		size_t computeInputIndex = 1;
		size_t computeOutputIndex = 2;
	};
}