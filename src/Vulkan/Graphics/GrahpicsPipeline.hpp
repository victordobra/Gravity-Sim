#pragma once

#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief Creates the Vulkan graphics pipeline.
	void CreateGraphicsPipeline();
	/// @brief Destroys the Vulkan graphics pipeline.
	void DestroyGraphicsPipeline();

	/// @brief Gets the Vulkan vertex shader module.
	/// @return The Vulkan vertex shader module.
	VkShaderModule GetVulkanVertexShaderModule();
	/// @brief Gets the Vulkan fragment shader module.
	/// @return The Vulkan fragment shader module.
	VkShaderModule GetVulkanFragmentShaderModule();
	/// @brief Gets the Vulkan graphics pipeline's layout.
	/// @return The Vulkan graphics pipeline's layout.
	VkPipelineLayout GetVulkanGraphicsPipelineLayout();
	/// @brief Gets the Vulkan graphics pipeline.
	/// @return The Vulkan graphics pipeline.
	VkPipeline GetVulkanGraphicsPipeline();

	/// @brief Draws the points to the screen.
	void DrawPoints();
}