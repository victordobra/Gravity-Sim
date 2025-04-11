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
		void CreateTreeBuffers();
		void CreateDescriptorPool();
		void CreateShaderModules();
		void CreatePipelines();
		void CreateCommandObjects();
		void RecordSecondaryCommandBuffers();

		VulkanDevice* device;
		ParticleSystem* particleSystem;

		VkBuffer countBuffer;
		VkBuffer radiusBuffer;
		VkBuffer nodePosBuffer;
		VkBuffer nodeMassBuffer;
		VkBuffer srcBuffer;
		VkDeviceMemory bufferMemory;

		VkBuffer treeCountBuffers[11];
		VkBuffer treeStartBuffers[11];
		VkBuffer treePosBuffers[11];
		VkBuffer treeMassBuffers[11];
		VkDeviceMemory treeBufferMemory;

		VkDescriptorSetLayout particleSetLayout;
		VkDescriptorSetLayout barnesHutSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSets[4];

		VkShaderModule clearShader;
		VkShaderModule forceShader;
		VkShaderModule initShader;
		VkShaderModule particleSortShader;
		VkShaderModule treeInitShader;
		VkShaderModule treeMoveShader;
		VkShaderModule treeSortShader;

		VkPipelineLayout bufferPipelineLayout;
		VkPipelineLayout treePipelineLayout;

		VkPipeline clearPipeline;
		VkPipeline forcePipeline;
		VkPipeline initPipeline;
		VkPipeline particleSortPipeline;
		VkPipeline treeInitPipeline;
		VkPipeline treeMovePipeline;
		VkPipeline treeSortPipeline;

		VkFence simulationFence;
		VkCommandBuffer commandBuffers[2];
		uint32_t commandBufferIndex = 0;

		VkCommandBuffer treeCommandBuffer;
	};
}