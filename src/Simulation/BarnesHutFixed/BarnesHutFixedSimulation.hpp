#pragma once

#include "Particles/ParticleSystem.hpp"
#include "Simulation/Simulation.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A particle simulation which uses the Barnes-Hut algorithm.
	class BarnesHutFixedSimulation : public Simulation {
	public:
		/// @brief Aligns the particle count corresponding to the fixed Barnes-Hut implementation's requirements.
		/// @param particleCount The original particle count.
		/// @return The aligned particle count.
		static size_t GetAlignedParticleCount(size_t particleCount);
		/// @brief Gets the size of the particle buffer required by the fixed Barnes-Hut implementation, considering the number of particles.
		/// @param alignedParticleCount The aligned particle count.
		/// @return The required particle buffer size.
		static size_t GetBufferSize(size_t alignedParticleCount);

		BarnesHutFixedSimulation() = delete;
		BarnesHutFixedSimulation(const BarnesHutFixedSimulation&) = delete;
		BarnesHutFixedSimulation(BarnesHutFixedSimulation&&) noexcept = delete;

		/// @brief Creates a particle simulation which uses the Barnes-Hut algorithm.
		/// @param device The Vulkan device to create the compute pipeline in.
		/// @param particleSystem The particle system whose particles to simulate.
		BarnesHutFixedSimulation(VulkanDevice* device, ParticleSystem* particleSystem);

		BarnesHutFixedSimulation& operator=(const BarnesHutFixedSimulation&) = delete;
		BarnesHutFixedSimulation& operator=(BarnesHutFixedSimulation&&) = delete;

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
		void RunSimulations(uint32_t simulationCount) override;

		/// @brief Destroys the Barnes-Hut simulation.
		~BarnesHutFixedSimulation();
	private:
		static const uint32_t TREE_DEPTH = 12;

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

		VkBuffer treeCountBuffers[TREE_DEPTH];
		VkBuffer treeStartBuffers[TREE_DEPTH];
		VkBuffer treePosBuffers[TREE_DEPTH];
		VkBuffer treeMassBuffers[TREE_DEPTH];
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