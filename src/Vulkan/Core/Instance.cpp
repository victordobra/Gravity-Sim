#include "Instance.hpp"
#include "Allocator.hpp"
#include "ProjectInfo.hpp"
#include "Debug/Logger.hpp"
#include <stdint.h>
#include <string.h>
#include <vector>
#include <unordered_set>

// Include Vulkan with the current platform define
#if defined(GSIM_PLATFORM_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(GSIM_PLATFORM_LINUX)
#define VK_USE_PLATFORM_XCB_KHR
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// String comparison function struct
	struct StrEqual {
		bool operator()(const char* str1, const char* str2) const {
			return !strcmp(str1, str2);
		}
	};
	typedef std::unordered_set<const char*, std::hash<const char*>, StrEqual> ExtensionUSet;

	// Constants
#if defined(GSIM_PLATFORM_WINDOWS)
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(GSIM_PLATFORM_LINUX)
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

	static const uint32_t MIN_API_VERSION = VK_API_VERSION_1_0;

	static const std::vector<const char*> MANDATORY_EXTENTIONS = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME
	};
	static const std::vector<const char*> OPTIONAL_EXTENSIONS = {
		
	};
	static const std::vector<const char*> MANDATORY_DEBUG_EXTENSIONS = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	static const std::vector<const char*> OPTIONAL_DEBUG_EXTENSIONS = {
		
	};

	static const std::vector<const char*> DEBUG_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

	// Variables
	static uint32_t apiVersion;
	static std::vector<const char*> instanceExtensions;
	static std::vector<const char*> instanceLayers;

	static bool debugEnabled;

	static VkInstance instance;
	static VkDebugUtilsMessengerEXT debugMessenger;

	// Debug message callback
	static VkBool32 DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		// Output a message with the given severity
		switch(messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			GSIM_LOG_DEBUG("Vulkan validation: %s", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			GSIM_LOG_INFO("Vulkan validation: %s", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			GSIM_LOG_WARNING("Vulkan validation: %s", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			GSIM_LOG_ERROR("Vulkan validation: %s", pCallbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}

	// Internal helper functions
	static bool CheckForExtensionSupport() {
		// Load the instance extension count
		uint32_t extensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		
		// Exit the function if no extensions are supported
		if(!extensionCount)
			return false;

		// Allocate the extension array and get all instance extensions
		VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
		if(!extensions)
			GSIM_LOG_FATAL("Failed to allocate instance extension array!");

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

		// Loop through every available extension, checking if it is mandatory, optional or for debugging
		uint32_t mandatoryExtensionCount = 0, mandatoryDebugExtensionCount = 0;

		VkExtensionProperties* extensionsEnd = extensions + extensionCount;
		for(VkExtensionProperties* extension = extensions; extension != extensionsEnd; ++extension) {
			// Check if the current extension is mandatory
			for(const char* mandatoryExtension : MANDATORY_EXTENTIONS)
				if(!strncmp(extension->extensionName, mandatoryExtension, VK_MAX_EXTENSION_NAME_SIZE)) {
					// Increment the mandatory extension count and insert the current extension in the instance extension list
					++mandatoryExtensionCount;
					instanceExtensions.push_back(mandatoryExtension);

					continue;
				}

			// Check if the current extension is optional
			for(const char* optionalExtension : OPTIONAL_EXTENSIONS)
				if(!strncmp(extension->extensionName, optionalExtension, VK_MAX_EXTENSION_NAME_SIZE)) {
					// Insert the current extension in the instance extension list
					instanceExtensions.push_back(optionalExtension);

					continue;
				}
			
			// Only proceed if debugging is enabled
			if(debugEnabled) {
				// Check if the current extension is mandatory for debugging
				for(const char* mandatoryDebugExtension : MANDATORY_DEBUG_EXTENSIONS)
					if(!strncmp(extension->extensionName, mandatoryDebugExtension, VK_MAX_EXTENSION_NAME_SIZE)) {
						// Increment the debug extension count
						++mandatoryDebugExtensionCount;

						continue;
					}
			}
		}

		// Check if all mandatory debug extensions are supported
		if(mandatoryDebugExtensionCount == MANDATORY_DEBUG_EXTENSIONS.size()) {
			// Insert all supported debug extensions in the instance extension vector
			for(VkExtensionProperties* extension = extensions; extension != extensionsEnd; ++extension) {
				// Check if the current extension is mandatory for debugging
				for(const char* mandatoryDebugExtension : MANDATORY_DEBUG_EXTENSIONS)
					if(!strncmp(extension->extensionName, mandatoryDebugExtension, VK_MAX_EXTENSION_NAME_SIZE)) {
						// Insert the current extension in the instance extension list
						instanceExtensions.push_back(mandatoryDebugExtension);

						continue;
					}

				// Check if the current extension is optional for debugging
				for(const char* optionalDebugExtension : OPTIONAL_DEBUG_EXTENSIONS)
					if(!strncmp(extension->extensionName, optionalDebugExtension, VK_MAX_EXTENSION_NAME_SIZE)) {
						// Insert the current extension in the instance extension list
						instanceExtensions.push_back(optionalDebugExtension);

						continue;
					}
			}
		} else if(debugEnabled) {
			// Disable debugging and warn the user
			debugEnabled = false;
			GSIM_LOG_WARNING("Vulkan debugging requested, but not supported!");
		}

		// Free the allocated extension list memory
		free(extensions);

		// Check if all mandatory extensions are supported
		return mandatoryExtensionCount == MANDATORY_EXTENTIONS.size();
	}
	static bool CheckForLayerSupport() {
		// Exit the function if debugging is not enabled
		if(!debugEnabled)
			return false;

		// Load the instance layer count
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		// Exit if no layers are available
		if(!layerCount)
			return false;

		// Allocate the layer array and get all instance layers
		VkLayerProperties* layers = (VkLayerProperties*)malloc(layerCount * sizeof(VkLayerProperties));
		if(!layers)
			GSIM_LOG_FATAL("Failed to allocate instance layer array!");

		vkEnumerateInstanceLayerProperties(&layerCount, layers);

		// Loop through every available layer, checking if it is wanted
		size_t debugLayerCount = 0;

		VkLayerProperties* layersEnd = layers + layerCount;
		for(VkLayerProperties* layer = layers; layer != layersEnd; ++layer) {
			// Check if the current layer is wanted
			for(const char* debugLayer : DEBUG_LAYERS)
				if(!strncmp(layer->layerName, debugLayer, VK_MAX_EXTENSION_NAME_SIZE)) {
					// Increment the debug layer count and insert the current layer in the layer list
					++debugLayerCount;
					instanceLayers.push_back(debugLayer);

					continue;
				}
		}

		// Free the layer array
		free(layers);

		// Return true if any layer was found
		return debugLayerCount;
	}

	static bool CheckForVulkanSupport() {
		// Check if the Vulkan API version is too low
		auto enumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");

		if(enumerateInstanceVersion)
			enumerateInstanceVersion(&apiVersion);
		else
			apiVersion = VK_API_VERSION_1_0;

		if(apiVersion < MIN_API_VERSION)
			return false;

		// Check if all required extensions are supported
		if(!CheckForExtensionSupport())
			return false;

		// Disable debugging and erase all debug extensions if no validation layers are supported
		if(debugEnabled && !CheckForLayerSupport()) {
			// Disable debugging
			debugEnabled = false;
		}

		return true;
	}
	static bool CreateInstance() {
		// Set the application info
		VkApplicationInfo appInfo;

		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = GSIM_PROJECT_NAME;
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, GSIM_PROJECT_VERSION_MAJOR, GSIM_PROJECT_VERSION_MINOR, GSIM_PROJECT_VERSION_PATCH);
		appInfo.pEngineName = nullptr;
		appInfo.engineVersion = 0;
		appInfo.apiVersion = apiVersion;

		// Set the debug messenger create info (if debugging is enabled)
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

		if(debugEnabled) {
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.pNext = nullptr;
			debugCreateInfo.flags = 0;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
			debugCreateInfo.pfnUserCallback = DebugMessageCallback;
			debugCreateInfo.pUserData = nullptr;
		}

		// Set the instance create info
		VkInstanceCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		// Extend the create info with the debug create info for instance creation validation
		if(debugEnabled)
			createInfo.pNext = &debugCreateInfo;
		else
			createInfo.pNext = nullptr;

		createInfo.flags = 0;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
		createInfo.ppEnabledLayerNames = instanceLayers.data();
		createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		// Create the instance
		VkResult result = vkCreateInstance(&createInfo, GetVulkanAllocCallbacks(), &instance);
		if(result == VK_ERROR_INCOMPATIBLE_DRIVER)
			return false;
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan instance! Error code: %s", string_VkResult(result));
		
		GSIM_LOG_INFO("Created Vulkan instance.");
		
		// Exit the function if debugging is disabled
		if(!debugEnabled)
			return true;
		
		// Reset the debug messenger create info pNext pointer
		debugCreateInfo.pNext = nullptr;
		
		// Get the create debug messenger callback
		PFN_vkCreateDebugUtilsMessengerEXT createMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if(!createMessenger)
			GSIM_LOG_FATAL("Failed to create Vulkan debug messenger! Messenger creation callback not found.");
		
		// Create the debug messenger
		result = createMessenger(instance, &debugCreateInfo, GetVulkanAllocCallbacks(), &debugMessenger);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan debug messenger! Error code: %s", string_VkResult(result));
		
		GSIM_LOG_INFO("Created Vulkan debug messenger.");
		
		return true;
	}

	// Public functions
	bool CreateVulkanInstance(bool enableDebugging) {
		// Set whether debugging is enabled or not
		debugEnabled = enableDebugging;

		// Check for Vulkan support
		if(!CheckForVulkanSupport())
			return false;
		
		// Try to create the Vulkan instance
		if(!CreateInstance())
			return false;

		return true;
	}
	void DestroyVulkanInstance() {
		// Destroy the Vulkan debug messenger, if debugging is enabled
		if(debugEnabled) {
			// Get the destroy debug messenger callback
			PFN_vkDestroyDebugUtilsMessengerEXT destroyMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if(!destroyMessenger)
				GSIM_LOG_FATAL("Failed to destroy Vulkan debug messenger! Messenger deletion callback not found.");
			
			// Destroy the debug messenger
			destroyMessenger(instance, debugMessenger, GetVulkanAllocCallbacks());
		}

		// Destroy the Vulkan instance
		vkDestroyInstance(instance, GetVulkanAllocCallbacks());
	}

	VkInstance GetVulkanInstance() {
		return instance;
	}
	uint32_t GetVulkanAPIVersion() {
		return apiVersion;
	}
	const std::vector<const char*>& GetVulkanInstanceExtensions() {
		return instanceExtensions;
	}
	const std::vector<const char*>& GetVulkanInstanceLayers() {
		return instanceLayers;
	}
}
