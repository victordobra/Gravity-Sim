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
	static const char* const REQUIRED_DEVICE_EXTENSIONS[] {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
	};
	static const size_t REQUIRED_DEVICE_EXTENSION_COUNT = sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(const char*);

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

		if(surface) {
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
	static bool CheckPhysicalDeviceSupport(VkPhysicalDevice physicalDevice, VulkanSurface* surface, VkPhysicalDeviceProperties2& properties2) {
		// Check if the physical device's version is high enough
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		if(properties.apiVersion <= VK_API_VERSION_1_1)
			return false;

		// Get the physical device's full properties
		vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

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
		for(size_t i = 0; i != REQUIRED_DEVICE_EXTENSION_COUNT; ++i) {
			// Check if the current extension is in the supported extensions array
			const char* extension = REQUIRED_DEVICE_EXTENSIONS[i];
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

		// Set the subgroup properties info struct
		VkPhysicalDeviceSubgroupProperties subgroupProperties {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES,
			.pNext = nullptr
		};
		VkPhysicalDeviceProperties2 properties2 {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = &subgroupProperties
		};

		// Check if any physical devices support all requirements, preferrably a distrete device
		for(uint32_t i = 0; i != physicalDeviceCount; ++i) {
			// Skip the device if it is not supported
			if(!CheckPhysicalDeviceSupport(physicalDevices[i], surface, properties2))
				continue;
			
			// Set the new best physical device
			physicalDevice = physicalDevices[i];

			// Ste the physical device's properties and subgroup size
			properties = properties2.properties;
			subgroupSize = subgroupProperties.subgroupSize;

			// Exit the loop if the current supported device is discrete
			if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				break;
		}

		// Free the physical device array
		free(physicalDevices);

		// Throw an exception if no physical device was found
		if(!physicalDevice)
			GSIM_THROW_EXCEPTION("Failed to find suitable Vulkan physical device!");
		
		// Get the physical device's remaining properties
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
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
		float queuePriorities[4]{ 1.0f, 1.0f, 1.0f, 1.0f };

		uint32_t familyIndexArr[4]{ indices.graphicsIndex, indices.presentIndex, indices.transferIndex, indices.computeIndex };
		uint32_t infoIndices[4];
		for(uint32_t i = 0; i != 4; ++i) {
			// Skip the current queue family index if it is not valid
			if(familyIndexArr[i] == UINT32_MAX)
				continue;
			
			// Check if the current queue family is already in use
			bool exists = false;
			for(uint32_t j = 0; j != queueInfoCount && !exists; ++j) {
				if(queueInfos[j].queueFamilyIndex == familyIndexArr[i]) {
					// Check if there is still room for another queue in the current family
					if(families[familyIndexArr[i]].queueCount) {
						--families[familyIndexArr[i]].queueCount;
						++queueInfos[j].queueCount;
					}

					infoIndices[i] = j;
					exists = true;
				}
			}

			// Check if an identical queue info doesn't already exist
			if(!exists) {
				// Set a new queue info
				queueInfos[queueInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfos[queueInfoCount].pNext = nullptr;
				queueInfos[queueInfoCount].flags = 0;
				queueInfos[queueInfoCount].queueFamilyIndex = familyIndexArr[i];
				queueInfos[queueInfoCount].queueCount = 1;
				queueInfos[queueInfoCount].pQueuePriorities = queuePriorities;
				--families[familyIndexArr[i]].queueCount;

				infoIndices[i] = queueInfoCount;
				++queueInfoCount;

				//Aadd the queue family index to the array
				indexArr[indexArrSize++] = familyIndexArr[i];
			}
		}

		// Set the required float atomic features
		VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT,
			.pNext = nullptr,
			.shaderBufferFloat32Atomics = VK_TRUE,
    		.shaderBufferFloat32AtomicAdd = VK_TRUE,
    		.shaderBufferFloat64Atomics = VK_FALSE,
    		.shaderBufferFloat64AtomicAdd = VK_FALSE,
    		.shaderSharedFloat32Atomics = VK_FALSE,
    		.shaderSharedFloat32AtomicAdd = VK_FALSE,
    		.shaderSharedFloat64Atomics = VK_FALSE,
    		.shaderSharedFloat64AtomicAdd = VK_FALSE,
    		.shaderImageFloat32Atomics = VK_FALSE,
    		.shaderImageFloat32AtomicAdd = VK_FALSE,
    		.sparseImageFloat32Atomics = VK_FALSE,
    		.sparseImageFloat32AtomicAdd = VK_FALSE
		};

		// Set the device's create info
		VkDeviceCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &atomicFloatFeatures,
			.flags = 0,
			.queueCreateInfoCount = queueInfoCount,
			.pQueueCreateInfos = queueInfos,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = REQUIRED_DEVICE_EXTENSION_COUNT,
			.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS,
			.pEnabledFeatures = &features
		};

		// Create the device
		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan logical device! Error code: %s", string_VkResult(result));
		
		// Reset the queue counts
		for(uint32_t i = 0; i != 4; ++i) {
			if(familyIndexArr[i] != UINT32_MAX)
				families[familyIndexArr[i]].queueCount = 0;
		}

		// Get the device queues
		if(indices.graphicsIndex != UINT32_MAX)
			vkGetDeviceQueue(device, indices.graphicsIndex, families[indices.graphicsIndex].queueCount++, &graphicsQueue);
		if(indices.presentIndex != UINT32_MAX)
			vkGetDeviceQueue(device, indices.presentIndex, families[indices.presentIndex].queueCount++, &presentQueue);

		vkGetDeviceQueue(device, indices.transferIndex, families[indices.transferIndex].queueCount++, &transferQueue);
		vkGetDeviceQueue(device, indices.computeIndex, families[indices.computeIndex].queueCount++, &computeQueue);

		// Free the queue families array
		free(families);

		// Create the graphics command pool, if graphics are used
		if(indices.graphicsIndex != UINT32_MAX) {
			VkCommandPoolCreateInfo graphicsCommandPoolInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = indices.graphicsIndex
			};

			result = vkCreateCommandPool(device, &graphicsCommandPoolInfo, nullptr, &graphicsCommandPool);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan graphics command pool! Error code: %s", string_VkResult(result));
		}

		// Create the transfer command pool
		VkCommandPoolCreateInfo transferCommandPoolInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = indices.transferIndex
		};

		result = vkCreateCommandPool(device, &transferCommandPoolInfo, nullptr, &transferCommandPool);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan transfer command pool! Error code: %s", string_VkResult(result));

		// Create the compute command pool
		VkCommandPoolCreateInfo computeCommandPoolInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = indices.computeIndex
		};

		result = vkCreateCommandPool(device, &computeCommandPoolInfo, nullptr, &computeCommandPool);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan compute command pool! Error code: %s", string_VkResult(result));
	}

	void VulkanDevice::LogDeviceInfo(Logger* logger) {
		// Log the physical device's name and type
		logger->LogMessage(Logger::MESSAGE_LEVEL_INFO, "Using Vulkan device: %s, type: %s", properties.deviceName, string_VkPhysicalDeviceType(properties.deviceType));

		// Log the Vulkan version in use
		logger->LogMessage(Logger::MESSAGE_LEVEL_INFO, "Vulkan version: %u.%u.%u", VK_API_VERSION_MAJOR(properties.apiVersion), VK_API_VERSION_MINOR(properties.apiVersion), VK_API_VERSION_PATCH(properties.apiVersion));

		// Log the queue family indices
		if(indices.graphicsIndex != UINT32_MAX) {
			logger->LogMessage(Logger::MESSAGE_LEVEL_INFO, "Vulkan device queue family indices: graphics - %u, present - %u, transfer - %u, compute - %u (%u unique)", indices.graphicsIndex, indices.presentIndex, indices.transferIndex, indices.computeIndex, (uint32_t)indexArrSize);
		} else {
			logger->LogMessage(Logger::MESSAGE_LEVEL_INFO, "Vulkan device queue family indices: transfer - %u, compute - %u (%u unique)", indices.transferIndex, indices.computeIndex, (uint32_t)indexArrSize);
		}
	}

	uint32_t VulkanDevice::GetMemoryTypeIndex(VkMemoryPropertyFlags propertyFlags, uint32_t memoryTypeBits) {
		// Loop through all supported memory types
		for(uint32_t i = 0; i != memoryProperties.memoryTypeCount; ++i) {
			// Skip the current memory type if its index is not set in the bitmask
			if(!((1 << i) & memoryTypeBits))
				continue;
			
			// Return the current memory type if all property flags are supported
			if((propertyFlags & memoryProperties.memoryTypes[i].propertyFlags) == propertyFlags)
				return i;
		}

		// No soutable memory type was found; return UINT32_MAX
		return UINT32_MAX;
	}

	VulkanDevice::~VulkanDevice() {
		// Destroy all command pools
		if(graphicsCommandPool)
			vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
		
		vkDestroyCommandPool(device, transferCommandPool, nullptr);
		vkDestroyCommandPool(device, computeCommandPool, nullptr);

		// Destroy the logical device
		vkDestroyDevice(device, nullptr);
	}
}