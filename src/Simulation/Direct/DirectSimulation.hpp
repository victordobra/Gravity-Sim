#pragma once

#include "Particles/ParticleSystem.hpp"
#include "Simulation/Simulation.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A particle simulation which uses the direct sum method.
	class DirectSimulation : public Simulation {
	public:
		/// @brief Gets the particle alignment required for the simulation to run.
		/// @return The particle alignment required for the simulation to run.
		static size_t GetRequiredParticleAlignment();

		DirectSimulation() = delete;
		DirectSimulation(const DirectSimulation&) = delete;
		DirectSimulation(DirectSimulation&&) noexcept = delete;
		
		/// @brief Creates a particle simulation which uses the direct sum method.
		/// @param device The Vulkan device to create the compute pipeline in.
		/// @param particleSystem The particle system whose particles to simulate.
		DirectSimulation(VulkanDevice* device, ParticleSystem* particleSystem);

		DirectSimulation& operator=(const DirectSimulation&) = delete;
		DirectSimulation& operator=(DirectSimulation&&) noexcept = delete;

		/// @brief Gets the Vulkan device that owns the compute pipeline.
		/// @return A pointer to the Vulkan device wrapper object.
		VulkanDevice* GetDevice() {
			return device;
		}
		/// @brief Gets the Vulkan device that owns the compute pipeline.
		/// @return A const pointer to the Vulkan device wrapper object.
		const VulkanDevice* GetDevice() const {
			return device;
		}
		/// @brief Gets the particle system whose particles to simulate.
		/// @return A pointer to the particle system object.
		ParticleSystem* GetParticleSystem() {
			return particleSystem;
		}
		/// @brief Gets the particle system whose particles to simulate.
		/// @return A const pointer to the particle system object.
		const ParticleSystem* GetParticleSystem() const {
			return particleSystem;
		}

		/// @brief Gets the Vulkan compute pipeline.
		/// @return A handle to the Vulkan compute pipeline.
		VkPipeline GetPipeline() {
			return pipeline;
		}
		/// @brief Gets the Vulkan fence used to synchronize simulations.
		/// @return A handle to the Vulkan fence used to synchronize simulations.
		VkFence GetSimulationFence() {
			return simulationFence;
		}

		/// @brief Runs the given number of simulations.
		/// @param simulationCount The number of simulations to run.
		void RunSimulations(uint32_t simulationCount);

		/// @brief Destroys the direct simulation.
		~DirectSimulation();
	private:
		VulkanDevice* device;
		ParticleSystem* particleSystem;

		VkDescriptorSetLayout setLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSets[3];
		VkShaderModule shaderModule;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		VkFence simulationFence;
		VkCommandBuffer commandBuffers[2];
		uint32_t commandBufferIndex = 0;
	};
}