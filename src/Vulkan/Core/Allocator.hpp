#pragma once

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief Gets the Vulkan allocation callbacks.
	/// @return A pointer to the Vulkan allocation callbacks struct.
	const VkAllocationCallbacks* GetVulkanAllocCallbacks();
}