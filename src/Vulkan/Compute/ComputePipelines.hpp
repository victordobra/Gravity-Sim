#pragma once

#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief Creates the Vulkan compute pipelines.
	void CreateComputePipelines();
	/// @brief Destroys the Vulkan compute pipelines.
	void DestroyComputePipelines();

	/// @brief Gets the Vulkan compute pipeline layout.
	/// @return A handle to the Vulkan compute pipeline layout.
	VkPipelineLayout GetVulkanComputePipelineLayout();
	/// @brief Gets the Vulkan compute gravity pipeline.
	/// @return A handle to the Vulkan compute gravity pipeline.
	VkPipeline GetVulkanComputeGravityPipeline();
	/// @brief Gets the Vulkan compute velocity pipeline.
	/// @return A handle to the Vulkan compute velocity pipeline.
	VkPipeline GetVulkanComputeVelocityPipeline();

	/// @brief Simulates gravityfor the current delta time.
	void SimulateGravity();
}