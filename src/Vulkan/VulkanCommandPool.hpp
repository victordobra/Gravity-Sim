#pragma once

#include "VulkanDevice.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A wrapper for a Vulkan command pool.
	class VulkanCommandPool {
	public:
		VulkanCommandPool() = delete;
		VulkanCommandPool(const VulkanCommandPool&) = delete;
		VulkanCommandPool(VulkanCommandPool&&) noexcept = delete;

		/// @brief Creates a Vulkan command pool.
		/// @param device The device to create the command pool in.
		/// @param queueFamilyIndex The queue family index the command pool will be bound to.
		/// @param createFlags The create flags for the command pool.
		VulkanCommandPool(VulkanDevice* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags);

		VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
		VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;

		/// @brief Gets the Vulkan device that owns this command pool.
		/// @return A pointer to the Vulkan device wrapper object.
		VulkanDevice* GetDevice() {
			return device;
		}
		/// @brief Gets the Vulkan device that owns this command pool.
		/// @return A const pointer to the Vulkan device wrapper object.
		const VulkanDevice* GetDevice() const {
			return device;
		}
		/// @brief Gets the Vulkan command pool of the implementation.
		/// @return A handle to the Vulkan command pool.
		VkCommandPool GetCommandPool() {
			return commandPool;
		}

		/// @brief Destroys the Vulkan command pool.
		~VulkanCommandPool();
	private:
		VulkanDevice* device;
		VkCommandPool commandPool;
	};
}