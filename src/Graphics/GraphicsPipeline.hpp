#pragma once

#include "Particles/ParticleSystem.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanSwapChain.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A graphics pipeline which renders particles.
	class GraphicsPipeline {
	public:
		GraphicsPipeline() = delete;
		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline(GraphicsPipeline&&) noexcept = delete;

		/// @brief Creates the graphics pipeline used to draw points to the screen.
		/// @param device The Vulkan device to create the pipeline in.
		/// @param swapChain The Vulkan swap chain to present the rendered images to.
		/// @param particleSystem The particle system containing the particles to render.
		GraphicsPipeline(VulkanDevice* device, VulkanSwapChain* swapChain, ParticleSystem* particleSystem);

		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = delete;

		/// @brief Gets the Vulkan device that owns the pipeline.
		/// @return A pointer to the Vulkan device wrapper object.
		VulkanDevice* GetDevice() {
			return device;
		}
		/// @brief Gets the Vulkan device that owns the pipeline.
		/// @return A const pointer to the Vulkan device wrapper object.
		const VulkanDevice* GetDevice() const {
			return device;
		}
		/// @brief Gets the Vulkan swap chain the rendered images are presented to.
		/// @return A pointer to the Vulkan swap chain wrapper object.
		VulkanSwapChain* GetSwapChain() {
			return swapChain;
		}
		/// @brief Gets the Vulkan swap chain the rendered images are presented to.
		/// @return A const pointer to the Vulkan swap chain wrapper object.
		const VulkanSwapChain* GetSwapChain() const {
			return swapChain;
		}
		/// @brief Gets the particle system whose particles to render.
		/// @return A pointer to the particle system object.
		ParticleSystem* GetParticleSystem() {
			return particleSystem;
		}
		/// @brief Gets the particle system whose particles to render.
		/// @return A const pointer to the particle system object.
		const ParticleSystem* GetParticleSystem() const {
			return particleSystem;
		}

		/// @brief Gets the Vulkan vertex shader module used by the pipeline.
		/// @return A handle to the Vulkan vertex shader module used by the pipeline.
		VkShaderModule GetVertexShaderModule() {
			return vertexShaderModule;
		}
		/// @brief Gets the Vulkan fragment shader module used by the pipeline.
		/// @return A handle to the Vulkan fragment shader module used by the pipeline.
		VkShaderModule GetFragmentShaderModule() {
			return fragmentShaderModule;
		}
		/// @brief Gets the Vulkan pipeline's layout.
		/// @return A handle to the Vulkan pipeline's layout.
		VkPipelineLayout GetPipelineLayout() {
			return pipelineLayout;
		}
		/// @brief Gets the Vulkan pipeline used by the implementation.
		/// @return A handle to the Vulkan pipeline.
		VkPipeline GetPipeline() {
			return pipeline;
		}
		/// @brief Gets the Vulkan fence used to synchronise rendering operations.
		/// @return A handle to the Vulkan rendering fence.
		VkFence GetRenderingFence() {
			return renderingFence;
		}

		/// @brief Renders the system's particles.
		/// @param cameraPos The camera's position.
		/// @param cameraSize The camera frustum's size.
		void RenderParticles(Vec2 cameraPos, Vec2 cameraSize);

		/// @brief Destroys the graphics pipeline.
		~GraphicsPipeline();
	private:
		VulkanDevice* device;
		VulkanSwapChain* swapChain;
		ParticleSystem* particleSystem;

		VkShaderModule vertexShaderModule;
		VkShaderModule fragmentShaderModule;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		VkFence renderingFence;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore* renderingFinishedSemaphores;
		VkCommandBuffer commandBuffer;
	};
}