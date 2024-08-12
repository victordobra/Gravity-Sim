#include "VulkanDevice.hpp"
#include "Debug/Exception.hpp"
#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	static const char* const REQUIRED_DEVICE_RENDERING_EXTENSIONS[] {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	static const size_t REQUIRED_DEVICE_RENDERING_EXTENSION_COUNT = sizeof(REQUIRED_DEVICE_RENDERING_EXTENSIONS) / sizeof(const char*);

	// Internal functions
	static VulkanDevice::QueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice physicalDevice, VulkanSurface* surface) {
		// Get the number of queue families
		uint32_t familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

		// Allocate the queue family array
		VkQueueFamilyProperties* families = (VkQueueFamilyProperties*)malloc(familyCount * sizeof(VkQueueFamilyProperties));
		if(!families)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan queue family array!");
		
		// Get all queue families
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families);

		// Find the best graphics and present queue family index, if required
		VulkanDevice::QueueFamilyIndices indices;

		for(uint32_t i = 0; i != familyCount; ++i) {
			// Check if the current queue supports graphics
			bool graphicsSupport = families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

			// Check if the current queue supports presenting
			VkBool32 presentSupport;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface->GetSurface(), &presentSupport);
#if defined(WIN32)
			presentSupport = presentSupport && vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, i);
#endif

			// Exit the loop if both graphics and presenting are supported, as this is the best possible scenario
			if(graphicsSupport && presentSupport && families[i].queueCount != 1) {
				indices.graphicsIndex = i;
				indices.presentIndex = i;
				break;
			}

			// Set the individual queue family indices
			if(graphicsSupport && indices.graphicsIndex == UINT32_MAX) {
				indices.graphicsIndex = i;
			}
			if(presentSupport && indices.presentIndex == UINT32_MAX) {
				indices.presentIndex = i;
			}
		}

		// Find the best transfer queue family index
		uint32_t maxScore = 0;
		for(uint32_t i = 0; i != familyCount; ++i) {
			// Skip the current family if it doesn't support transfer
			if(!(families[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
				continue;

			// Calculate the current family's score
			uint32_t score = 1;
			score += ((i != indices.graphicsIndex) + (i != indices.presentIndex)) << 1;
			score += (i == indices.graphicsIndex && families[i].queueCount != 1);
			score += (i == indices.presentIndex && families[i].queueCount != 1);

			// Check if the current score is the maximum score
			if(score > maxScore) {
				maxScore = score;
				indices.transferIndex = i;
			}
		}

		// Find the best compute queue family index
		maxScore = 0;
		for(uint32_t i = 0; i != familyCount; ++i) {
			// Skip the current family if it doesn't support compute pipelines
			if(!(families[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
				continue;
			
			// Calculate the current family's score
			uint32_t score = 1;
			score += ((i != indices.graphicsIndex) + (i != indices.presentIndex) + (i != indices.transferIndex)) * 3;
			score += (i == indices.graphicsIndex && families[i].queueCount != 1);
			score += (i == indices.presentIndex && families[i].queueCount != 1);
			score += (i == indices.transferIndex && families[i].queueCount != 1);

			// Check if the current score is the maximum score
			if(score > maxScore) {
				maxScore = score;
				indices.computeIndex = i;
			}
		}

		// Free the queue family info array
		free(families);

		return indices;
	}
	static bool CheckPhysicalDeviceSupport(VkPhysicalDevice physicalDevice, VulkanSurface* surface) {
		// Check for rendering support, if required
		if(surface) {
			// Get the number of supported extensions
			uint32_t supportedExtensionCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &supportedExtensionCount, nullptr);

			// Allocate the supported extension array
			VkExtensionProperties* supportedExtensions = (VkExtensionProperties*)malloc(supportedExtensionCount * sizeof(VkExtensionProperties));
			if(!supportedExtensions)
				GSIM_THROW_EXCEPTION("Failed to allocate Vulkan supported device extensions array!");
			
			// Get all supported extensions
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &supportedExtensionCount, supportedExtensions);

			// Check if all rendering extensions are supported
			for(size_t i = 0; i != REQUIRED_DEVICE_RENDERING_EXTENSION_COUNT; ++i) {
				// Check if the current extension is in the supported extensions array
				const char* extension = REQUIRED_DEVICE_RENDERING_EXTENSIONS[i];
				bool supported = false;

				for(uint32_t j = 0; j != supportedExtensionCount && !supported; ++j)
					supported = !strncmp(extension, supportedExtensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE);
				
				// Exit the function if the extension is not supported
				if(!supported) {
					free(supportedExtensions);
					return false;
				}
			}

			// Free the supported extension array
			free(supportedExtensions);
		}

		// Get the device's queue family indices
		VulkanDevice::QueueFamilyIndices indices = FindQueueFamilyIndices(physicalDevice, surface);

		// Check if all required indices are supported
		if(surface && (indices.graphicsIndex == UINT32_MAX || indices.presentIndex == UINT32_MAX))
			return false;
		return indices.transferIndex != UINT32_MAX && indices.computeIndex != UINT32_MAX;
	}
	
	// Public functions
	VulkanDevice::VulkanDevice(VulkanInstance* instance, VulkanSurface* surface) : instance(instance), physicalDevice(VK_NULL_HANDLE) {
		// Get the number of physical devices
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(instance->GetInstance(), &physicalDeviceCount, nullptr);

		// Allocate the physical device array
		VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
		if(!physicalDevices)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan physical device array!");
		
		// Get all physical devices
		vkEnumeratePhysicalDevices(instance->GetInstance(), &physicalDeviceCount, physicalDevices);

		// Check if any physical devices support all requirements, preferrably a distrete device
		for(uint32_t i = 0; i != physicalDeviceCount; ++i) {
			// Skip the device if it is not supported
			if(!CheckPhysicalDeviceSupport(physicalDevices[i], surface))
				continue;
			
			// Set the new best physical device
			physicalDevice = physicalDevices[i];

			// Get the physical device's properties
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);

			// Exit the loop if the current supported device is discrete
			if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				break;
		}

		// Free the physical device array
		free(physicalDevices);

		// Throw an exception if no physical device was found
		if(!physicalDevice)
			GSIM_THROW_EXCEPTION("Failed to find suitable Vulkan physical device!");
		
		// Get the physical device's features and queue family indices
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);
		indices = FindQueueFamilyIndices(physicalDevice, surface);

		// Get the number of queue families
		uint32_t familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

		// Allocate the queue family array
		VkQueueFamilyProperties* families = (VkQueueFamilyProperties*)malloc(familyCount * sizeof(VkQueueFamilyProperties));
		if(!families)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan queue family array!");
		
		// Get all queue families
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families);

		// Set all queue family create infos
		uint32_t queueInfoCount = 0;
		VkDeviceQueueCreateInfo queueInfos[4];
		float queuePriorities[4]{ 1.f, 1.f, 1.f, 1.f };

		uint32_t indexArr[4]{ indices.graphicsIndex, indices.presentIndex, indices.transferIndex, indices.computeIndex };
		uint32_t infoIndices[4];
		for(uint32_t i = 0; i != 4; ++i) {
			// Skip the current queue family index if it is not valid
			if(indexArr[i] == UINT32_MAX)
				continue;
			
			// Check if the current queue family is already in use
			bool exists = false;
			for(uint32_t j = 0; j != queueInfoCount && !exists; ++j) {
				if(queueInfos[j].queueFamilyIndex == indexArr[i]) {
					// Check if there is still room for another queue in the current family
					if(families[indexArr[i]].queueCount) {
						--families[indexArr[i]].queueCount;
						++queueInfos[j].queueCount;
					}

					infoIndices[i] = j;
					exists = true;
				}
			}

			// Set a new queue info if one doesn't already exist
			if(!exists) {
				queueInfos[queueInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfos[queueInfoCount].pNext = nullptr;
				queueInfos[queueInfoCount].flags = 0;
				queueInfos[queueInfoCount].queueFamilyIndex = indexArr[i];
				queueInfos[queueInfoCount].queueCount = 1;
				queueInfos[queueInfoCount].pQueuePriorities = queuePriorities;
				--families[indexArr[i]].queueCount;

				infoIndices[i] = queueInfoCount;
				++queueInfoCount;
			}
		}

		// Set the enabled extension array
		uint32_t extensionCount;
		const char* const* extensions;
		if(surface) {
			extensionCount = (uint32_t)REQUIRED_DEVICE_RENDERING_EXTENSION_COUNT;
			extensions = REQUIRED_DEVICE_RENDERING_EXTENSIONS;
		} else {
			extensionCount = 0;
			extensions = nullptr;
		}

		// Set the device's create info
		VkDeviceCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = queueInfoCount,
			.pQueueCreateInfos = queueInfos,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = extensionCount,
			.ppEnabledExtensionNames = extensions,
			.pEnabledFeatures = &features
		};

		// Create the device
		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan logical device! Error code: %s", string_VkResult(result));
		
		// Reset the queue counts
		for(uint32_t i = 0; i != 4; ++i) {
			if(indexArr[i] != UINT32_MAX)
				families[indexArr[i]].queueCount = 0;
		}

		// Get the device queues
		if(indices.graphicsIndex != UINT32_MAX)
			vkGetDeviceQueue(device, indices.graphicsIndex, families[indices.graphicsIndex].queueCount++, &graphicsQueue);
		if(indices.presentIndex != UINT32_MAX)
			vkGetDeviceQueue(device, indices.presentIndex, families[indices.presentIndex].queueCount++, &presentQueue);
		if(indices.transferIndex != UINT32_MAX)
			vkGetDeviceQueue(device, indices.transferIndex, families[indices.transferIndex].queueCount++, &transferQueue);
		if(indices.computeIndex != UINT32_MAX)
			vkGetDeviceQueue(device, indices.computeIndex, families[indices.computeIndex].queueCount++, &computeQueue);

		// Free the queue families array
		free(families);
	}

	VulkanDevice::~VulkanDevice() {
		// Wait for the device to idle and destroy it
		vkDeviceWaitIdle(device);
		vkDestroyDevice(device, nullptr);
	}
}