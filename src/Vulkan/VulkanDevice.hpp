#pragma once

#include "VulkanInstance.hpp"
#include "VulkanSurface.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A wrapper for a Vulkan logical device.
	class VulkanDevice {
	public:
		/// @brief A struct containing the queue family indices to be used by the device.
		struct QueueFamilyIndices {
			/// @brief The graphics queue family's index, or UINT32_MAX if one wasn't found.
			uint32_t graphicsIndex = UINT32_MAX;
			/// @brief The present queue family's index, or UINT32_MAX if one wasn't found.
			uint32_t presentIndex = UINT32_MAX;
			/// @brief The transfer queue family's index, or UINT32_MAX if one wasn't found.
			uint32_t transferIndex = UINT32_MAX;
			/// @brief The compute queue family's index, or UINT32_MAX if one wasn't found.
			uint32_t computeIndex = UINT32_MAX;
		};

		VulkanDevice() = delete;
		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice(VulkanDevice&&) noexcept = delete;

		/// @brief Creates a Vulkan device.
		/// @param instance The Vulkan instance to create the device in.
		/// @param surface The Vulkan surface the device should support, or a nullptr if rendering is not required.
		VulkanDevice(VulkanInstance* instance, VulkanSurface* surface);

		VulkanDevice& operator=(const VulkanDevice&) = delete;
		VulkanDevice& operator=(VulkanDevice&&) = delete;

		/// @brief Gets the Vulkan instance that owns the device.
		/// @return A pointer to the Vulkan instance wrapper object.
		VulkanInstance* GetInstance() {
			return instance;
		}
		/// @brief Gets the Vulkan instance that owns the device.
		/// @return A const pointer to the Vulkan instance wrapper object.
		const VulkanInstance* GetInstance() const {
			return instance;
		}

		/// @brief Gets the Vulkan logical device of the implementation.
		/// @return A handle to the Vulkan logical device.
		VkDevice GetDevice() {
			return device;
		}
		/// @brief Gets the Vulkan physical device of the implementation.
		/// @return A handle to the Vulkan physical device.
		VkPhysicalDevice GetPhysicalDevice() {
			return physicalDevice;
		}
		/// @brief Gets the queue family indices used by the Vulkan device.
		/// @return A struct containing the queue family indices.
		const QueueFamilyIndices& GetQueueFamilyIndices() const {
			return indices;
		}
		/// @brief Gets the Vulkan physical device's properties.
		/// @return A struct containing the physical device's properties.
		const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const {
			return properties;
		}
		/// @brief Gets the Vulkan physical device's properties.
		/// @return A struct containing the physical device's properties.
		const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatures() const {
			return features;
		}

		/// @brief Gets the Vulkan logical device's graphics queue.
		/// @return A handle to the graphics queue, or VK_NULL_HANDLE if the device isn't uded for graphics.
		VkQueue GetGraphicsQueue() {
			return graphicsQueue;
		}
		/// @brief Gets the Vulkan logical device's present queue.
		/// @return A handle to the present queue, or VK_NULL_HANDLE if the device isn't uded for graphics.
		VkQueue GetPresentQueue() {
			return presentQueue;
		}
		/// @brief Gets the Vulkan logical device's transfer queue.
		/// @return A handle to the transfer queue.
		VkQueue GetTransferQueue() {
			return transferQueue;
		}
		/// @brief Gets the Vulkan logical device's compute queue.
		/// @return A handle to the compute queue.
		VkQueue GetComputeQueue() {
			return computeQueue;
		}

		/// @brief Destroys the Vulkan device.
		~VulkanDevice();
	private:
		VulkanInstance* instance;

		VkDevice device;
		VkPhysicalDevice physicalDevice;
		QueueFamilyIndices indices;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		VkQueue transferQueue;
		VkQueue computeQueue;
	};
}