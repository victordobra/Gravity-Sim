#pragma once

#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief Creates the Vulkan sync objects.
	void CreateVulkanSyncObjects();
	/// @brief Destroys the Vulkan sync objects.
	void DestroyVulkanSyncObjects();

	/// @brief Gets the Vulkan fence used by graphics operations.
	/// @return A handle to the Vulkan graphics fence.
	VkFence GetVulkanGraphicsFence();
	/// @brief Gets the Vulkan fence used by compute operations.
	/// @return A handle to the Vulkan compute fence.
	VkFence GetVulkanComputeFence();

	/// @brief Waits for the Vulkan graphics and compute fences.
	void WaitForVulkanFences();
}