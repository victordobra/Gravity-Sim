#include "VulkanInstance.hpp"
#include "Debug/Exception.hpp"
#include "ProjectInfo.hpp"
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
	static const char* const REQUIRED_INSTANCE_EXTENSIONS[] = {
#if defined(WIN32)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME
	};
	static const size_t REQUIRED_INSTANCE_EXTENSION_COUNT = sizeof(REQUIRED_INSTANCE_EXTENSIONS) / sizeof(const char*);

	static const char* const REQUIRED_INSTANCE_DEBUG_EXTENSIONS[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	static const size_t REQUIRED_INSTANCE_DEBUG_EXTENSION_COUNT = sizeof(REQUIRED_INSTANCE_DEBUG_EXTENSIONS) / sizeof(const char*);

	static const char* const REQUIRED_INSTANCE_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	static const size_t REQUIRED_INSTANCE_LAYER_COUNT = sizeof(REQUIRED_INSTANCE_LAYERS) / sizeof(const char*);

	// Debug messenger callback
	static VkBool32 DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		// Get the logger from the user data pointer
		Logger* logger = (Logger*)pUserData;

		// Log the message based on its severity
		switch(messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			logger->LogMessage(Logger::MESSAGE_LEVEL_INFO, "Vulkan validation: %s", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			logger->LogMessage(Logger::MESSAGE_LEVEL_WARNING, "Vulkan validation: %s", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			logger->LogMessage(Logger::MESSAGE_LEVEL_ERROR, "Vulkan validation: %s", pCallbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}

	// Public functions
	VulkanInstance::VulkanInstance(bool validationEnabled, Logger* logger) {
		// Get the number of supported extensions and layers
		uint32_t supportedExtensionCount, supportedLayerCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

		// Allocate the supported extension and layer arrays
		VkExtensionProperties* supportedExtensions = (VkExtensionProperties*)malloc(supportedExtensionCount * sizeof(VkExtensionProperties) + supportedLayerCount * sizeof(VkLayerProperties));
		if(!supportedExtensions)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan supported extension and layer arrays!");
		
		VkLayerProperties* supportedLayers = (VkLayerProperties*)(supportedExtensions + supportedExtensionCount);

		// Get all supported extensions and layers
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions);
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers);

		// Check if all required instance extensions are supported
		for(size_t i = 0; i != REQUIRED_INSTANCE_EXTENSION_COUNT; ++i) {
			// Check if the current extension is in the supported extension array
			const char* extension = REQUIRED_INSTANCE_EXTENSIONS[i];
			bool supported = false;

			for(uint32_t j = 0; j != supportedExtensionCount && !supported; ++j)
				supported = !strncmp(extension, supportedExtensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE);
			
			// Throw an exception if the extension is not supported
			if(!supported)
				GSIM_THROW_EXCEPTION("Required Vulkan instance extension %s is not supported!", extension);
		}

		// Check if all required debug extensions and validation layers are supported, if requested
		bool validationRequested = validationEnabled;

		for(size_t i = 0; i != REQUIRED_INSTANCE_DEBUG_EXTENSION_COUNT && validationEnabled; ++i) {
			// Check if the current extension is in the supported extension array
			const char* extension = REQUIRED_INSTANCE_DEBUG_EXTENSIONS[i];
			bool supported = false;

			for(uint32_t j = 0; j != supportedExtensionCount && !supported; ++j)
				supported = !strncmp(extension, supportedExtensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE);
			
			// Disable validation if the extension is not supported
			validationEnabled = supported;
		}
		for(size_t i = 0; i != REQUIRED_INSTANCE_LAYER_COUNT && validationEnabled; ++i) {
			// Check if the current layer is in the supported layer array
			const char* layer = REQUIRED_INSTANCE_LAYERS[i];
			bool supported = false;

			for(uint32_t j = 0; j != supportedLayerCount && !supported; ++j)
				supported = !strncmp(layer, supportedLayers[j].layerName, VK_MAX_EXTENSION_NAME_SIZE);
			
			// Disable validation if the layer is not supported
			validationEnabled = supported;
		}

		if(validationRequested && !validationEnabled)
			logger->LogMessage(Logger::MESSAGE_LEVEL_WARNING, "Vulkan validation requested, but not supported! Validation messages will not be shown.");
		
		// Free the supported extension and layer arrays
		free(supportedExtensions);
		
		// Set the used extension and layer arrays
		const char** extensions;
		uint32_t extensionCount;
		if(validationEnabled) {
			// Set the extension count
			extensionCount = (uint32_t)(REQUIRED_INSTANCE_EXTENSION_COUNT + REQUIRED_INSTANCE_DEBUG_EXTENSION_COUNT);

			// Allocate the extension array
			extensions = (const char**)malloc(extensionCount * sizeof(const char*));
			if(!extensions)
				GSIM_THROW_EXCEPTION("Failed to allocate Vulkan instance extension array!");
			
			// Set the extensions in the array
			for(size_t i = 0; i != REQUIRED_INSTANCE_EXTENSION_COUNT; ++i)
				extensions[i] = REQUIRED_INSTANCE_EXTENSIONS[i];
			for(size_t i = 0; i != REQUIRED_INSTANCE_DEBUG_EXTENSION_COUNT; ++i)
				extensions[i + REQUIRED_INSTANCE_EXTENSION_COUNT] = REQUIRED_INSTANCE_DEBUG_EXTENSIONS[i];
		} else {
			// Set the extension count and extensions array
			extensionCount = (uint32_t)REQUIRED_INSTANCE_EXTENSION_COUNT;
			extensions = (const char**)REQUIRED_INSTANCE_EXTENSIONS;
		}

		const char** layers;
		uint32_t layerCount;
		if(validationEnabled) {
			layerCount = (uint32_t)REQUIRED_INSTANCE_LAYER_COUNT;
			layers = (const char**)REQUIRED_INSTANCE_LAYERS;
		} else {
			layerCount = 0;
			layers = nullptr;
		}

		// Set the debug messenger info
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = 0,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = DebugMessengerCallback,
			.pUserData = logger
		};

		// Set the app info
		VkApplicationInfo appInfo {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = GSIM_PROJECT_NAME,
			.applicationVersion = VK_MAKE_API_VERSION(0, GSIM_PROJECT_VERSION_MAJOR, GSIM_PROJECT_VERSION_MINOR, GSIM_PROJECT_VERSION_PATCH),
			.pEngineName = nullptr,
			.engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
			.apiVersion = VK_API_VERSION_1_3
		};

		// Set the instance info
		VkInstanceCreateInfo instanceInfo {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = layerCount,
			.ppEnabledLayerNames = layers,
			.enabledExtensionCount = extensionCount,
			.ppEnabledExtensionNames = extensions
		};

		// Add the debug messenger create info to the pNext chain, if validation is enabled
		if(validationEnabled)
			instanceInfo.pNext = &debugMessengerInfo;
		
		// Create the instance
		VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan instance! Error code: %s", string_VkResult(result));
		
		// Create the debug messenger, if requested
		if(validationEnabled) {
			auto pfnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			result = pfnCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan debug utils messenger! Error code: %s", string_VkResult(result));
		} else {
			debugMessenger = VK_NULL_HANDLE;
		}

		// Free the extension array, if allocated
		if(validationEnabled) {
			free(extensions);
		}
	}

    VulkanInstance::~VulkanInstance() {
		// Destroy the debug messenger, if it exists
		if(debugMessenger) {
			auto pfnDestroyDebugUrilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			pfnDestroyDebugUrilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		
		// Destroy the instance
		vkDestroyInstance(instance, nullptr);
	}
}