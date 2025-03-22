#pragma once

#include "Particles/ParticleSystem.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A particle simulation which uses the Barnes-Hut algorithm.
	class BarnesHutSimulation {
	public:
		/// @brief Gets the particle alignment required for the simulation to run.
		/// @return The particle alignment required for the simulation to run.
		static size_t GetRequiredParticleAlignment();

		BarnesHutSimulation() = delete;
		BarnesHutSimulation(const BarnesHutSimulation&) = delete;
		BarnesHutSimulation(BarnesHutSimulation&&) noexcept = delete;

		/// @brief Creates a particle simulation which uses the Barnes-Hut algorithm.
		/// @param device The Vulkan device to create the compute pipeline in.
		/// @param particleSystem The particle system whose particles to simulate.
		BarnesHutSimulation(VulkanDevice* device, ParticleSystem* particleSystem);

		BarnesHutSimulation& operator=(const BarnesHutSimulation&) = delete;
		BarnesHutSimulation& operator=(BarnesHutSimulation&&) = delete;

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

		/// @brief Runs the given number of simulations.
		/// @param simulationCount The number of simulations to run.
		void RunSimulations(uint32_t simulationCount);

		/// @brief Destroys the Barnes-Hut simulation.
		~BarnesHutSimulation();
	private:
		void CreateBuffers();
		void CreateDescriptorPool();
		void CreateShaderModules();
		void CreatePipelines();
		void CreateCommandObjects();
		void ClearBuffers();

		VulkanDevice* device;
		ParticleSystem* particleSystem;

		VkBuffer stateBuffer;
		VkBuffer treeBuffer;
		VkBuffer intBuffer;
		VkDeviceMemory bufferMemory;

		VkDescriptorSetLayout particleSetLayout;
		VkDescriptorSetLayout barnesHutSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSets[4];

		VkShaderModule boxShader;
		VkShaderModule treeShader;
		VkShaderModule centerShader;
		VkShaderModule sortShader;
		VkShaderModule forceShader;
		VkShaderModule accelShader;

		VkPipelineLayout pipelineLayout;
		VkPipeline boxPipeline;
		VkPipeline treePipeline;
		VkPipeline centerPipeline;
		VkPipeline sortPipeline;
		VkPipeline forcePipeline;
		VkPipeline accelPipeline;

		VkFence simulationFence;
		VkCommandBuffer commandBuffers[2];
		uint32_t commandBufferIndex = 0;
	};
}