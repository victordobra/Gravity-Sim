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
		/// @brief Gets the Vulkan physical device's memory properties.
		/// @return A struct containing the physical device's memory properties.
		const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties() const {
			return memoryProperties;
		}
		/// @brief Gets the Vulkan physical device's properties.
		/// @return A struct containing the physical device's properties.
		const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatures() const {
			return features;
		}

		/// @brief Gets the size of the array containing all unique queue family indices.
		/// @return The size of the array containing all unique queue family indices.
		uint32_t GetQueueFamilyIndexArraySize() const {
			return indexArrSize;
		}
		/// @brief Gets the array containing all unique queue family indices.
		/// @return A pointer to the array containing all unique queue family indices.
		const uint32_t* GetQueueFamilyIndexArray() const {
			return indexArr;
		}

		/// @brief Gets the Vulkan logical device's graphics queue.
		/// @return A handle to the graphics queue, or VK_NULL_HANDLE if the device isn't used for graphics.
		VkQueue GetGraphicsQueue() {
			return graphicsQueue;
		}
		/// @brief Gets the Vulkan logical device's present queue.
		/// @return A handle to the present queue, or VK_NULL_HANDLE if the device isn't used for graphics.
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

		/// @brief Gets the Vulkan command pool bound to the graphics queue.
		/// @return A handle to the graphics command pool, or VK_NULL_HANDLE if the device isn't used for graphics.
		VkCommandPool GetGraphicsCommandPool() {
			return graphicsCommandPool;
		}
		/// @brief Gets the Vulkan command pool bound to the transfer queue.
		/// @return A handle to the transfer command pool.
		VkCommandPool GetTransferCommandPool() {
			return transferCommandPool;
		}
		/// @brief Gets the Vulkan command pool bound to the compute queue.
		/// @return A handle to the compute command pool.
		VkCommandPool GetComputeCommandPool() {
			return computeCommandPool;
		}

		/// @brief Finds a memory type with all required properties.
		/// @param propertyFlags The property flags required for the memory type.
		/// @param memoryTypeBits The bitmask in which the index of the memory type must be set.
		/// @return The index of the first memory type with all required properties, or UINT32_MAX if no such memory type exists.
		uint32_t GetMemoryTypeIndex(VkMemoryPropertyFlags propertyFlags, uint32_t memoryTypeBits);

		/// @brief Destroys the Vulkan device.
		~VulkanDevice();
	private:
		VulkanInstance* instance;

		VkDevice device;
		VkPhysicalDevice physicalDevice;
		QueueFamilyIndices indices;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		VkPhysicalDeviceFeatures features;

		uint32_t indexArrSize = 0;
		uint32_t indexArr[4];

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		VkQueue transferQueue;
		VkQueue computeQueue;

		VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
		VkCommandPool transferCommandPool;
		VkCommandPool computeCommandPool;
	};
}