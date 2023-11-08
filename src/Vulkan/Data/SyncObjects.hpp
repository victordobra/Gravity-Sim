#pragma once

#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief Creates the Vulkan sync objects.
	void CreateVulkanSyncObjects();
	/// @brief Destroys the Vulkan sync objects.
	void DestroyVulkanSyncObjects();

	/// @brief Gets the sync fences used for the point buffers. 
	/// @return An array of POINT_BUFFER_COUNT length containing all buffer fences.
	VkFence* GetVulkanPointBufferFences();
	/// @brief Gets the sync semaphores used for the point buffers. 
	/// @return An array of POINT_BUFFER_COUNT length containing all buffer semaphores.
	VkSemaphore* GetVulkanPointBufferSemaphores();
}